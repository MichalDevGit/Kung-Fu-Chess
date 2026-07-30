#include "GameNodeRequestPublisher.h"

#include <sw/redis++/redis++.h>

#include "GameNodeMessages.h"

GameNodeRequestPublisher::GameNodeRequestPublisher(sw::redis::Redis& redis) : redis(redis)
{
}

void GameNodeRequestPublisher::forward(const std::string& channel, const std::string& connectionId, const std::string& rawJson)
{
    redis.publish(channel, GameNodeRequest{"client_message", connectionId, rawJson, 0}.toJson().dump());
}

void GameNodeRequestPublisher::notifyConnectionClosed(const std::string& channel, const std::string& connectionId)
{
    redis.publish(channel, GameNodeRequest{"connection_closed", connectionId, "", 0}.toJson().dump());
}

void GameNodeRequestPublisher::requestReconnectCheck(const std::string& channel, const std::string& connectionId, int userId)
{
    redis.publish(channel, GameNodeRequest{"reconnect_check", connectionId, "", userId}.toJson().dump());
}

void GameNodeRequestPublisher::requestSessionCreation(
    const std::string& channel,
    const std::string& sessionId,
    int whiteUserId,
    const std::string& whiteUsername,
    const std::string& whiteConnectionId,
    int blackUserId,
    const std::string& blackUsername,
    const std::string& blackConnectionId)
{
    redis.publish(
        channel,
        GameNodeCreateSessionRequest{
            sessionId, whiteUserId, whiteUsername, whiteConnectionId, blackUserId, blackUsername, blackConnectionId}
            .toJson()
            .dump());
}
