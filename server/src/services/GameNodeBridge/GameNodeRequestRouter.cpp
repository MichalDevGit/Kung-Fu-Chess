#include "GameNodeRequestRouter.h"

#include <chrono>
#include <thread>
#include <utility>

#include <nlohmann/json.hpp>
#include <sw/redis++/redis++.h>

#include "GameNodeMessages.h"
#include "GameNodePushPublisher.h"
#include "IReconnectResolver.h"
#include "common/Logging/Logger.h"
#include "handlers/GameRequestHandler.h"
#include "services/GameSession/GameSessionManager.h"

GameNodeRequestRouter::GameNodeRequestRouter(
    sw::redis::Redis& redis,
    std::string channel,
    GameRequestHandler& gameHandler,
    GameSessionManager& sessionManager,
    IReconnectResolver& reconnectResolver,
    GameNodePushPublisher& pushPublisher)
    : gameHandler(gameHandler)
    , sessionManager(sessionManager)
    , reconnectResolver(reconnectResolver)
    , pushPublisher(pushPublisher)
    , redis(redis)
    , channel(std::move(channel))
{
}

void GameNodeRequestRouter::onMessage(const std::string& /*channel*/, const std::string& payload)
{
    nlohmann::json parsed;
    try
    {
        parsed = nlohmann::json::parse(payload);
    }
    catch (const nlohmann::json::exception&)
    {
        return;
    }

    const std::string kind = parsed.value("kind", std::string());

    if (kind == "create_session")
    {
        const GameNodeCreateSessionRequest request = GameNodeCreateSessionRequest::fromJson(parsed);
        sessionManager.createSession(
            request.sessionId,
            GameSession::Player{request.whiteUserId, request.whiteUsername, request.whiteConnectionId},
            GameSession::Player{request.blackUserId, request.blackUsername, request.blackConnectionId});
        return;
    }

    const GameNodeRequest request = GameNodeRequest::fromJson(parsed);

    if (request.kind == "client_message")
    {
        const std::string reply = gameHandler.handle(request.connectionId, request.rawJson);
        pushPublisher.push(request.connectionId, reply);
        return;
    }

    if (request.kind == "connection_closed")
    {
        sessionManager.onConnectionClosed(request.connectionId);
        return;
    }

    if (request.kind == "reconnect_check")
    {
        const std::optional<protocol::MatchFoundResult> resume =
            reconnectResolver.checkAndRebind(request.userId, request.connectionId);
        pushPublisher.pushReconnectCheckResult(request.connectionId, resume.has_value() ? resume->toJson() : "");
        return;
    }
}

void GameNodeRequestRouter::start()
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
                    subscriber.subscribe(channel);

                    while (true)
                        subscriber.consume();
                }
                catch (const std::exception& exception)
                {
                    common::Logger::warn(std::string("GameNodeRequestRouter consume loop error: ") + exception.what());
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                }
            }
        })
        .detach();
}
