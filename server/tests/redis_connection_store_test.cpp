// Requires a real, reachable Redis -- KUNGFUCHESS_TEST_REDIS_URL, defaulting
// to docker-compose's `redis` service. Unlike the Postgres tests, this file
// isn't behind a compile-time macro (hiredis/redis-plus-plus are vendored
// unconditionally, see server/CMakeLists.txt), so it always compiles/runs;
// it just needs Redis reachable to pass, same as this project's own server
// would need it to actually use RedisConnectionStore.
#include "tests/doctest.h"
#include "services/ConnectionRegistry.h"
#include "services/RedisConnectionStore.h"

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
        redis.del("conn:test-conn-1");
        redis.del("conn:test-conn-2");
        redis.del("user_conn:9001");
    }

    ConnectionRegistry::AuthenticatedUser testUser()
    {
        return ConnectionRegistry::AuthenticatedUser{9001, "redis-alice", 1200};
    }
}

TEST_CASE("ConnectionRegistry(RedisConnectionStore) binds and finds a connection both directions")
{
    sw::redis::Redis redis(testRedisUrl());
    resetKeys(redis);
    RedisConnectionStore store(redis);
    ConnectionRegistry registry(store);

    registry.onAuthenticated("test-conn-1", testUser());

    const std::optional<ConnectionRegistry::AuthenticatedUser> found = registry.find("test-conn-1");
    REQUIRE(found.has_value());
    CHECK(found->userId == 9001);
    CHECK(found->username == "redis-alice");

    CHECK(*registry.findConnectionForUser(9001) == "test-conn-1");

    resetKeys(redis);
}

TEST_CASE("ConnectionRegistry(RedisConnectionStore): a re-authenticated user supersedes the old connection")
{
    sw::redis::Redis redis(testRedisUrl());
    resetKeys(redis);
    RedisConnectionStore store(redis);
    ConnectionRegistry registry(store);

    registry.onAuthenticated("test-conn-1", testUser());
    registry.onAuthenticated("test-conn-2", testUser());

    CHECK_FALSE(registry.find("test-conn-1").has_value());
    CHECK(registry.find("test-conn-2").has_value());
    CHECK(*registry.findConnectionForUser(9001) == "test-conn-2");

    resetKeys(redis);
}

TEST_CASE("ConnectionRegistry(RedisConnectionStore): onDisconnected erases both directions")
{
    sw::redis::Redis redis(testRedisUrl());
    resetKeys(redis);
    RedisConnectionStore store(redis);
    ConnectionRegistry registry(store);

    registry.onAuthenticated("test-conn-1", testUser());
    registry.onDisconnected("test-conn-1");

    CHECK_FALSE(registry.find("test-conn-1").has_value());
    CHECK_FALSE(registry.findConnectionForUser(9001).has_value());
}
