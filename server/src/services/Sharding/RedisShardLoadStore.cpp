#include "RedisShardLoadStore.h"

#include <chrono>

#include "common/Config/GameNodeConfig.h"

namespace
{
    const char* LOAD_HASH_KEY = "shard_load";
    std::string heartbeatKey(const std::string& shardId) { return "shard_heartbeat:" + shardId; }
}

RedisShardLoadStore::RedisShardLoadStore(sw::redis::Redis& redis) : redis(redis)
{
}

void RedisShardLoadStore::registerShard(const std::string& shardId)
{
    redis.hsetnx(LOAD_HASH_KEY, shardId, "0");

    // Heartbeat immediately, not just on this shard's own tick loop's next
    // scheduled refresh -- otherwise there's a real (if short) window right
    // after startup where this shard is "known" but not yet "alive",
    // which ShardHealthMonitor would misread as already dead.
    heartbeat(shardId);
}

void RedisShardLoadStore::incrementLoad(const std::string& shardId)
{
    redis.hincrby(LOAD_HASH_KEY, shardId, 1);
}

void RedisShardLoadStore::decrementLoad(const std::string& shardId)
{
    // Clamp at zero rather than letting a stray extra decrement (there's no
    // atomic "decrement but not below zero" primitive in Redis) send this
    // negative -- a negative load would make this shard look artificially
    // *more* attractive to pickShard() than a genuinely idle one.
    const long long newValue = redis.hincrby(LOAD_HASH_KEY, shardId, -1);
    if (newValue < 0)
        redis.hset(LOAD_HASH_KEY, shardId, "0");
}

std::vector<std::string> RedisShardLoadStore::knownShards() const
{
    std::vector<std::string> shards;
    redis.hkeys(LOAD_HASH_KEY, std::back_inserter(shards));
    return shards;
}

long long RedisShardLoadStore::loadOf(const std::string& shardId) const
{
    const std::optional<std::string> value = redis.hget(LOAD_HASH_KEY, shardId);
    return value.has_value() ? std::stoll(*value) : 0;
}

void RedisShardLoadStore::heartbeat(const std::string& shardId)
{
    redis.set(heartbeatKey(shardId), "1", std::chrono::milliseconds(GameNodeConfig::SHARD_HEARTBEAT_TTL_MILLIS));
}

bool RedisShardLoadStore::isAlive(const std::string& shardId) const
{
    return redis.exists(heartbeatKey(shardId)) > 0;
}

void RedisShardLoadStore::forget(const std::string& shardId)
{
    redis.hdel(LOAD_HASH_KEY, shardId);
    redis.del(heartbeatKey(shardId));
}
