// Requires a real, reachable Redis -- see redis_connection_store_test.cpp
// for why this isn't behind a compile-time macro.
#include "tests/doctest.h"
#include "services/Sharding/RedisGameShardRoutingStore.h"

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
        redis.del("shard_session:test-session-1");
        redis.del("shard_session:test-session-2");
        redis.del("shard_user:9001");
        redis.del("shard_user:9002");
        redis.del("shard_user:9003");
        redis.del("shard_user:9004");
        redis.del("shard_keys:test-session-1");
        redis.del("shard_keys:test-session-2");
        redis.del("shard_sessions:shard-1");
        redis.del("session_users:test-session-1");
        redis.del("session_users:test-session-2");
    }
}

TEST_CASE("RedisGameShardRoutingStore::bindSession indexes the session and both users")
{
    sw::redis::Redis redis(testRedisUrl());
    resetKeys(redis);
    RedisGameShardRoutingStore store(redis);

    store.bindSession("test-session-1", {9001, 9002}, "shard-1");

    CHECK(*store.findShardForSession("test-session-1") == "shard-1");
    CHECK(*store.findShardForUser(9001) == "shard-1");
    CHECK(*store.findShardForUser(9002) == "shard-1");

    resetKeys(redis);
}

TEST_CASE("RedisGameShardRoutingStore::unbindSession removes the session and user routing entries")
{
    sw::redis::Redis redis(testRedisUrl());
    resetKeys(redis);
    RedisGameShardRoutingStore store(redis);

    store.bindSession("test-session-1", {9001, 9002}, "shard-1");
    store.unbindSession("test-session-1", {9001, 9002});

    CHECK_FALSE(store.findShardForSession("test-session-1").has_value());
    CHECK_FALSE(store.findShardForUser(9001).has_value());
    CHECK_FALSE(store.findShardForUser(9002).has_value());
}

TEST_CASE("RedisGameShardRoutingStore::sessionsForShard/usersForSession report what bindSession recorded")
{
    sw::redis::Redis redis(testRedisUrl());
    resetKeys(redis);
    RedisGameShardRoutingStore store(redis);

    store.bindSession("test-session-1", {9001, 9002}, "shard-1");
    store.bindSession("test-session-2", {9003, 9004}, "shard-1");

    const std::vector<std::string> sessions = store.sessionsForShard("shard-1");
    CHECK(sessions.size() == 2);
    CHECK(std::count(sessions.begin(), sessions.end(), "test-session-1") == 1);
    CHECK(std::count(sessions.begin(), sessions.end(), "test-session-2") == 1);

    const std::vector<int> users = store.usersForSession("test-session-1");
    CHECK(users.size() == 2);
    CHECK(std::count(users.begin(), users.end(), 9001) == 1);
    CHECK(std::count(users.begin(), users.end(), 9002) == 1);

    resetKeys(redis);
}

TEST_CASE("RedisGameShardRoutingStore::unbindSession removes just that session from its shard's set")
{
    sw::redis::Redis redis(testRedisUrl());
    resetKeys(redis);
    RedisGameShardRoutingStore store(redis);

    store.bindSession("test-session-1", {9001, 9002}, "shard-1");
    store.bindSession("test-session-2", {9003, 9004}, "shard-1");

    store.unbindSession("test-session-1", {9001, 9002});

    const std::vector<std::string> sessions = store.sessionsForShard("shard-1");
    CHECK(sessions.size() == 1);
    CHECK(sessions[0] == "test-session-2");
    CHECK(store.usersForSession("test-session-1").empty());

    resetKeys(redis);
}
