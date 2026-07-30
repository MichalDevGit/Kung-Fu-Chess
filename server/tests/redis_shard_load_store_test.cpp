// Requires a real, reachable Redis -- see redis_connection_store_test.cpp
// for why this isn't behind a compile-time macro.
#include "tests/doctest.h"
#include "services/Sharding/RedisShardLoadStore.h"

#include <algorithm>
#include <cstdlib>
#include <sw/redis++/redis++.h>

namespace
{
    std::string testRedisUrl()
    {
        const char* url = std::getenv("KUNGFUCHESS_TEST_REDIS_URL");
        return url ? std::string(url) : "tcp://127.0.0.1:6379";
    }

    void resetKeys(sw::redis::Redis& redis)
    {
        redis.del("shard_load");
        redis.del("shard_heartbeat:shard-1");
    }
}

TEST_CASE("RedisShardLoadStore::registerShard is idempotent and starts a shard at zero load")
{
    sw::redis::Redis redis(testRedisUrl());
    resetKeys(redis);
    RedisShardLoadStore store(redis);

    store.registerShard("shard-1");
    store.registerShard("shard-1");

    CHECK(store.loadOf("shard-1") == 0);
    const std::vector<std::string> shards = store.knownShards();
    CHECK(std::count(shards.begin(), shards.end(), "shard-1") == 1);

    resetKeys(redis);
}

TEST_CASE("RedisShardLoadStore::incrementLoad/decrementLoad track a shard's live session count")
{
    sw::redis::Redis redis(testRedisUrl());
    resetKeys(redis);
    RedisShardLoadStore store(redis);

    store.registerShard("shard-1");
    store.incrementLoad("shard-1");
    store.incrementLoad("shard-1");
    CHECK(store.loadOf("shard-1") == 2);

    store.decrementLoad("shard-1");
    CHECK(store.loadOf("shard-1") == 1);

    resetKeys(redis);
}

TEST_CASE("RedisShardLoadStore::decrementLoad clamps at zero")
{
    sw::redis::Redis redis(testRedisUrl());
    resetKeys(redis);
    RedisShardLoadStore store(redis);

    store.registerShard("shard-1");
    store.decrementLoad("shard-1");

    CHECK(store.loadOf("shard-1") == 0);

    resetKeys(redis);
}

TEST_CASE("RedisShardLoadStore::registerShard heartbeats immediately, so a freshly registered shard is already alive")
{
    sw::redis::Redis redis(testRedisUrl());
    resetKeys(redis);
    RedisShardLoadStore store(redis);

    store.registerShard("shard-1");

    CHECK(store.isAlive("shard-1"));

    resetKeys(redis);
}

TEST_CASE("RedisShardLoadStore::isAlive is false for a shard that was never registered/heartbeaten")
{
    sw::redis::Redis redis(testRedisUrl());
    resetKeys(redis);
    RedisShardLoadStore store(redis);

    CHECK_FALSE(store.isAlive("shard-never-registered"));
}

TEST_CASE("RedisShardLoadStore::forget removes a shard from knownShards and clears its heartbeat")
{
    sw::redis::Redis redis(testRedisUrl());
    resetKeys(redis);
    RedisShardLoadStore store(redis);

    store.registerShard("shard-1");
    store.forget("shard-1");

    const std::vector<std::string> shards = store.knownShards();
    CHECK(std::count(shards.begin(), shards.end(), "shard-1") == 0);
    CHECK_FALSE(store.isAlive("shard-1"));
}
