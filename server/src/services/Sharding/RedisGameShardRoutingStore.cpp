#include "RedisGameShardRoutingStore.h"

namespace
{
    std::string sessionKey(const std::string& sessionId) { return "shard_session:" + sessionId; }
    std::string userKey(int userId) { return "shard_user:" + std::to_string(userId); }
    std::string sessionKeysKey(const std::string& sessionId) { return "shard_keys:" + sessionId; }
    std::string shardSessionsKey(const std::string& shardId) { return "shard_sessions:" + shardId; }
    std::string sessionUsersKey(const std::string& sessionId) { return "session_users:" + sessionId; }
}

RedisGameShardRoutingStore::RedisGameShardRoutingStore(sw::redis::Redis& redis) : redis(redis)
{
}

void RedisGameShardRoutingStore::trackKeyForSession(const std::string& sessionId, const std::string& key)
{
    redis.sadd(sessionKeysKey(sessionId), key);
}

void RedisGameShardRoutingStore::bindSession(
    const std::string& sessionId, const std::vector<int>& userIds, const std::string& shardId)
{
    const std::string sKey = sessionKey(sessionId);
    redis.set(sKey, shardId);
    trackKeyForSession(sessionId, sKey);

    for (int userId : userIds)
    {
        const std::string key = userKey(userId);
        redis.set(key, shardId);
        trackKeyForSession(sessionId, key);
    }

    const std::string usersKey = sessionUsersKey(sessionId);
    for (int userId : userIds)
        redis.sadd(usersKey, std::to_string(userId));
    trackKeyForSession(sessionId, usersKey);

    // Deliberately NOT tracked via trackKeyForSession -- this key is shared
    // across every session on this shard, so unbindSession must SREM just
    // this session's own membership from it, never blanket-DEL the whole
    // key the way every other tracked key above is.
    redis.sadd(shardSessionsKey(shardId), sessionId);
}

void RedisGameShardRoutingStore::unbindSession(const std::string& sessionId, const std::vector<int>& /*userIds*/)
{
    const std::optional<std::string> shardId = findShardForSession(sessionId);
    if (shardId.has_value())
        redis.srem(shardSessionsKey(*shardId), sessionId);

    const std::string keysKey = sessionKeysKey(sessionId);

    std::vector<std::string> keys;
    redis.smembers(keysKey, std::back_inserter(keys));

    for (const std::string& key : keys)
        redis.del(key);

    redis.del(keysKey);
}

std::optional<std::string> RedisGameShardRoutingStore::findShardForSession(const std::string& sessionId) const
{
    return redis.get(sessionKey(sessionId));
}

std::optional<std::string> RedisGameShardRoutingStore::findShardForUser(int userId) const
{
    return redis.get(userKey(userId));
}

std::vector<std::string> RedisGameShardRoutingStore::sessionsForShard(const std::string& shardId) const
{
    std::vector<std::string> sessions;
    redis.smembers(shardSessionsKey(shardId), std::back_inserter(sessions));
    return sessions;
}

std::vector<int> RedisGameShardRoutingStore::usersForSession(const std::string& sessionId) const
{
    std::vector<std::string> raw;
    redis.smembers(sessionUsersKey(sessionId), std::back_inserter(raw));

    std::vector<int> userIds;
    userIds.reserve(raw.size());
    for (const std::string& value : raw)
        userIds.push_back(std::stoi(value));

    return userIds;
}
