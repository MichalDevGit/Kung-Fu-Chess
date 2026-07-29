#include "GameNodeRequestPublisher.h"

#include <sw/redis++/redis++.h>

#include "GameNodeMessages.h"
#include "common/Config/GameNodeConfig.h"

GameNodeRequestPublisher::GameNodeRequestPublisher(sw::redis::Redis& redis) : redis(redis)
{
}

void GameNodeRequestPublisher::forward(const std::string& connectionId, const std::string& rawJson)
{
    redis.publish(GameNodeConfig::REQUESTS_CHANNEL, GameNodeRequest{"client_message", connectionId, rawJson, 0}.toJson().dump());
}

void GameNodeRequestPublisher::notifyConnectionClosed(const std::string& connectionId)
{
    redis.publish(
        GameNodeConfig::REQUESTS_CHANNEL, GameNodeRequest{"connection_closed", connectionId, "", 0}.toJson().dump());
}

void GameNodeRequestPublisher::requestReconnectCheck(const std::string& connectionId, int userId)
{
    redis.publish(
        GameNodeConfig::REQUESTS_CHANNEL,
        GameNodeRequest{"reconnect_check", connectionId, "", userId}.toJson().dump());
}
