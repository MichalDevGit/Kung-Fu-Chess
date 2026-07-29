#include "GameNodePushPublisher.h"

#include <sw/redis++/redis++.h>

#include "GameNodeMessages.h"
#include "common/Config/GameNodeConfig.h"

GameNodePushPublisher::GameNodePushPublisher(sw::redis::Redis& redis) : redis(redis)
{
}

void GameNodePushPublisher::push(const std::string& connectionId, const std::string& json)
{
    redis.publish(GameNodeConfig::PUSHES_CHANNEL, GameNodePush{"push", connectionId, json}.toJson().dump());
}

void GameNodePushPublisher::pushReconnectCheckResult(const std::string& connectionId, const std::string& matchFoundResultJson)
{
    redis.publish(
        GameNodeConfig::PUSHES_CHANNEL,
        GameNodePush{"reconnect_check_result", connectionId, matchFoundResultJson}.toJson().dump());
}
