// Requires a real, reachable Redis -- see redis_connection_store_test.cpp
// for why this isn't behind a compile-time macro.
#include "tests/doctest.h"
#include "services/SessionIndex/RedisSessionIndexStore.h"

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
        redis.del("session_conn:test-conn-1");
        redis.del("session_conn:test-conn-2");
        redis.del("session_user:9001");
        redis.del("session_user:9002");
        redis.del("session_keys:test-session-1");
    }
}

TEST_CASE("RedisSessionIndexStore::bindSession indexes both connections and users")
{
    sw::redis::Redis redis(testRedisUrl());
    resetKeys(redis);
    RedisSessionIndexStore store(redis);

    store.bindSession("test-session-1", {"test-conn-1", "test-conn-2"}, {9001, 9002});

    CHECK(*store.findSessionIdByConnection("test-conn-1") == "test-session-1");
    CHECK(*store.findSessionIdByConnection("test-conn-2") == "test-session-1");
    CHECK(*store.findSessionIdByUserId(9001) == "test-session-1");
    CHECK(*store.findSessionIdByUserId(9002) == "test-session-1");

    resetKeys(redis);
}

TEST_CASE("RedisSessionIndexStore::bindConnection rebinds a new connection to an existing session")
{
    sw::redis::Redis redis(testRedisUrl());
    resetKeys(redis);
    RedisSessionIndexStore store(redis);

    store.bindSession("test-session-1", {"test-conn-1"}, {9001});
    store.bindConnection("test-conn-2", "test-session-1");

    CHECK(*store.findSessionIdByConnection("test-conn-2") == "test-session-1");

    resetKeys(redis);
}

TEST_CASE("RedisSessionIndexStore::unbindConnection only drops the connection index")
{
    sw::redis::Redis redis(testRedisUrl());
    resetKeys(redis);
    RedisSessionIndexStore store(redis);

    store.bindSession("test-session-1", {"test-conn-1"}, {9001});
    store.unbindConnection("test-conn-1");

    CHECK_FALSE(store.findSessionIdByConnection("test-conn-1").has_value());
    CHECK(*store.findSessionIdByUserId(9001) == "test-session-1"); // untouched

    resetKeys(redis);
}

TEST_CASE("RedisSessionIndexStore::unbindSession removes every connection/user index for that session")
{
    sw::redis::Redis redis(testRedisUrl());
    resetKeys(redis);
    RedisSessionIndexStore store(redis);

    store.bindSession("test-session-1", {"test-conn-1", "test-conn-2"}, {9001, 9002});
    store.unbindSession("test-session-1");

    CHECK_FALSE(store.findSessionIdByConnection("test-conn-1").has_value());
    CHECK_FALSE(store.findSessionIdByConnection("test-conn-2").has_value());
    CHECK_FALSE(store.findSessionIdByUserId(9001).has_value());
    CHECK_FALSE(store.findSessionIdByUserId(9002).has_value());
}
