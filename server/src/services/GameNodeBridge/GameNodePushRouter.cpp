#include "GameNodePushRouter.h"

#include <thread>
#include <utility>

#include <nlohmann/json.hpp>
#include <sw/redis++/redis++.h>

#include "GameNodeMessages.h"
#include "common/Config/GameNodeConfig.h"
#include "common/Logging/Logger.h"

GameNodePushRouter::GameNodePushRouter(sw::redis::Redis& redis, PushHandler defaultHandler)
    : redis(redis), defaultHandler(std::move(defaultHandler))
{
}

void GameNodePushRouter::onMessage(const std::string& /*channel*/, const std::string& payload)
{
    GameNodePush push;
    try
    {
        push = GameNodePush::fromJson(nlohmann::json::parse(payload));
    }
    catch (const nlohmann::json::exception&)
    {
        // A malformed pub/sub payload shouldn't take the consume loop down --
        // same never-throw discipline every other handler in this codebase
        // already follows for a bad client message.
        return;
    }

    if (push.kind == "reconnect_check_result")
    {
        std::shared_ptr<Waiter> waiter;
        {
            std::lock_guard<std::mutex> lock(waitersMutex);
            const auto it = waiters.find(push.connectionId);
            if (it != waiters.end())
                waiter = it->second;
        }

        if (waiter)
        {
            std::lock_guard<std::mutex> lock(waiter->mutex);
            waiter->payload = push.json;
            waiter->ready = true;
            waiter->cv.notify_all();
        }
        return;
    }

    defaultHandler(push.connectionId, push.json);
}

void GameNodePushRouter::start()
{
    std::thread(
        [this]()
        {
            while (true)
            {
                try
                {
                    sw::redis::Subscriber subscriber = redis.subscriber();
                    subscriber.on_message([this](std::string channel, std::string payload)
                        { onMessage(channel, payload); });
                    subscriber.subscribe(GameNodeConfig::PUSHES_CHANNEL);

                    while (true)
                        subscriber.consume();
                }
                catch (const std::exception& exception)
                {
                    // A dropped Redis connection shouldn't kill this thread --
                    // re-subscribe and keep trying, same resilience spirit as
                    // WebSocketServer's per-send try/catch.
                    common::Logger::warn(std::string("GameNodePushRouter consume loop error: ") + exception.what());
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                }
            }
        })
        .detach();
}

std::optional<std::string> GameNodePushRouter::waitForReconnectCheckResult(
    const std::string& connectionId, long long timeoutMillis, const std::function<void()>& afterRegistered)
{
    auto waiter = std::make_shared<Waiter>();
    {
        std::lock_guard<std::mutex> lock(waitersMutex);
        waiters[connectionId] = waiter;
    }

    afterRegistered();

    std::unique_lock<std::mutex> lock(waiter->mutex);
    const bool arrived =
        waiter->cv.wait_for(lock, std::chrono::milliseconds(timeoutMillis), [&] { return waiter->ready; });
    const std::string payload = waiter->payload;
    lock.unlock();

    {
        std::lock_guard<std::mutex> outerLock(waitersMutex);
        waiters.erase(connectionId);
    }

    if (!arrived)
        return std::nullopt;

    return payload;
}
