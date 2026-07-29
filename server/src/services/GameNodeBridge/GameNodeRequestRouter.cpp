#include "GameNodeRequestRouter.h"

#include <chrono>
#include <thread>

#include <nlohmann/json.hpp>
#include <sw/redis++/redis++.h>

#include "GameNodeMessages.h"
#include "GameNodePushPublisher.h"
#include "IReconnectResolver.h"
#include "common/Config/GameNodeConfig.h"
#include "common/Logging/Logger.h"
#include "handlers/GameRequestHandler.h"
#include "handlers/MatchmakingRequestHandler.h"
#include "protocol/Message.h"
#include "protocol/MessageType.h"
#include "services/GameSession/GameSessionManager.h"
#include "services/Matchmaking/Matchmaker.h"

GameNodeRequestRouter::GameNodeRequestRouter(
    sw::redis::Redis& redis,
    MatchmakingRequestHandler& matchmakingHandler,
    GameRequestHandler& gameHandler,
    Matchmaker& matchmaker,
    GameSessionManager& sessionManager,
    IReconnectResolver& reconnectResolver,
    GameNodePushPublisher& pushPublisher)
    : matchmakingHandler(matchmakingHandler)
    , gameHandler(gameHandler)
    , matchmaker(matchmaker)
    , sessionManager(sessionManager)
    , reconnectResolver(reconnectResolver)
    , pushPublisher(pushPublisher)
    , redis(redis)
{
}

namespace
{
bool isUnknownType(const std::string& responseJson)
{
    const nlohmann::json parsed = nlohmann::json::parse(responseJson);
    return parsed.value("type", std::string()) == protocol::MessageType::Error &&
           parsed.value("error", std::string()) == "unknown_type";
}
}

std::string GameNodeRequestRouter::dispatchClientMessage(const std::string& connectionId, const std::string& rawJson) const
{
    const std::string matchmakingResponse = matchmakingHandler.handle(connectionId, rawJson);
    if (!isUnknownType(matchmakingResponse))
        return matchmakingResponse;

    return gameHandler.handle(connectionId, rawJson);
}

void GameNodeRequestRouter::onMessage(const std::string& /*channel*/, const std::string& payload)
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
        const std::string reply = dispatchClientMessage(request.connectionId, request.rawJson);
        pushPublisher.push(request.connectionId, reply);
        return;
    }

    if (request.kind == "connection_closed")
    {
        matchmaker.removeByConnection(request.connectionId);
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
                    subscriber.subscribe(GameNodeConfig::REQUESTS_CHANNEL);

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
