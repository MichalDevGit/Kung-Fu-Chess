#include "GameAllocatorRequestRouter.h"

#include <chrono>
#include <thread>

#include <nlohmann/json.hpp>
#include <sw/redis++/redis++.h>

#include "GameNodeMessages.h"
#include "GameNodePushPublisher.h"
#include "common/Config/GameNodeConfig.h"
#include "common/Logging/Logger.h"
#include "handlers/MatchmakingRequestHandler.h"
#include "services/Matchmaking/Matchmaker.h"

GameAllocatorRequestRouter::GameAllocatorRequestRouter(
    sw::redis::Redis& redis, MatchmakingRequestHandler& matchmakingHandler, Matchmaker& matchmaker, GameNodePushPublisher& pushPublisher)
    : matchmakingHandler(matchmakingHandler), matchmaker(matchmaker), pushPublisher(pushPublisher), redis(redis)
{
}

void GameAllocatorRequestRouter::onMessage(const std::string& /*channel*/, const std::string& payload)
{
    GameNodeRequest request;
    try
    {
        request = GameNodeRequest::fromJson(nlohmann::json::parse(payload));
    }
    catch (const nlohmann::json::exception&)
    {
        return;
    }

    if (request.kind == "client_message")
    {
        const std::string reply = matchmakingHandler.handle(request.connectionId, request.rawJson);
        pushPublisher.push(request.connectionId, reply);
        return;
    }

    if (request.kind == "connection_closed")
    {
        matchmaker.removeByConnection(request.connectionId);
        return;
    }
}

void GameAllocatorRequestRouter::start()
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
                    subscriber.subscribe(GameNodeConfig::MATCHMAKING_REQUESTS_CHANNEL);

                    while (true)
                        subscriber.consume();
                }
                catch (const std::exception& exception)
                {
                    common::Logger::warn(std::string("GameAllocatorRequestRouter consume loop error: ") + exception.what());
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                }
            }
        })
        .detach();
}
