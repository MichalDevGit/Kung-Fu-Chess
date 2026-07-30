#include "ShardHealthMonitor.h"

#include "IGameShardRoutingStore.h"
#include "IShardLoadStore.h"
#include "protocol/Message.h"

ShardHealthMonitor::ShardHealthMonitor(
    IShardLoadStore& loadStore, IGameShardRoutingStore& routingStore, FindConnectionForUserFn findConnectionForUser, PushFn push)
    : loadStore(loadStore)
    , routingStore(routingStore)
    , findConnectionForUser(std::move(findConnectionForUser))
    , push(std::move(push))
{
}

void ShardHealthMonitor::forfeitShard(const std::string& shardId)
{
    for (const std::string& sessionId : routingStore.sessionsForShard(shardId))
    {
        const std::vector<int> userIds = routingStore.usersForSession(sessionId);

        for (int userId : userIds)
        {
            const std::optional<std::string> connectionId = findConnectionForUser(userId);
            if (connectionId.has_value())
                push(*connectionId, protocol::GameOverMessage{"shard_unavailable", 0}.toJson());
        }

        routingStore.unbindSession(sessionId, userIds);
    }

    loadStore.forget(shardId);
}

void ShardHealthMonitor::checkAndForfeitDeadShards()
{
    for (const std::string& shardId : loadStore.knownShards())
    {
        if (!loadStore.isAlive(shardId))
            forfeitShard(shardId);
    }
}
