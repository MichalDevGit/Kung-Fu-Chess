// Requires a real, reachable Redis -- KUNGFUCHESS_TEST_REDIS_URL, defaulting
// to docker-compose's `redis` service, same convention as
// redis_connection_store_test.cpp/redis_match_queue_store_test.cpp/
// redis_session_index_store_test.cpp. Exercises the actual Redis pub/sub
// round trip GameNodePushRouter/GameNodePushPublisher use in production
// (MIGRATION_PLAN.md Phase 3) -- a fake/in-memory pub/sub double would prove
// nothing about the one part of this seam that's genuinely new risk (a
// background consume-loop thread, timing-sensitive waiter fulfillment).
//
// GameNodePushRouter::start() spawns a thread that's meant to run for the
// life of the process (same "detached, never stopped" contract
// server/src/main.cpp's tick thread and WebSocketServer's own accept loop
// already rely on -- see GameNodePushRouter.h). Constructing and destroying
// one per TEST_CASE would destroy its Redis connection out from under that
// still-running background thread (a real crash, not a hypothetical one --
// this file used to do exactly that and SIGSEGV'd on the second TEST_CASE's
// publish). A single, process-lifetime Fixture (function-local static, torn
// down only at process exit, well after every TEST_CASE has run) matches how
// this class is actually used in gamenode/src/main.cpp/server/src/main.cpp
// and sidesteps the problem instead of inventing a stop() this class doesn't
// need in production.
#include "tests/doctest.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <sw/redis++/redis++.h>

#include "services/GameNodeBridge/GameNodePushPublisher.h"
#include "services/GameNodeBridge/GameNodePushRouter.h"

namespace
{
std::string testRedisUrl()
{
    const char* url = std::getenv("KUNGFUCHESS_TEST_REDIS_URL");
    return url ? std::string(url) : "tcp://127.0.0.1:6379";
}

struct CapturedPush
{
    std::string connectionId;
    std::string json;
};

// Thread-safe collector for GameNodePushRouter's default handler -- it's
// invoked from the router's own background thread, never the test thread.
class PushCollector
{
public:
    void operator()(const std::string& connectionId, const std::string& json)
    {
        std::lock_guard<std::mutex> lock(mutex);
        pushes.push_back({connectionId, json});
        cv.notify_all();
    }

    bool waitForAny(std::chrono::milliseconds timeout)
    {
        std::unique_lock<std::mutex> lock(mutex);
        return cv.wait_for(lock, timeout, [&] { return !pushes.empty(); });
    }

    std::vector<CapturedPush> snapshot()
    {
        std::lock_guard<std::mutex> lock(mutex);
        return pushes;
    }

private:
    std::mutex mutex;
    std::condition_variable cv;
    std::vector<CapturedPush> pushes;
};

struct Fixture
{
    sw::redis::Redis redis;
    sw::redis::Redis publisherRedis;
    std::shared_ptr<PushCollector> collector = std::make_shared<PushCollector>();
    GameNodePushPublisher publisher;
    GameNodePushRouter router;

    Fixture()
        : redis(testRedisUrl())
        , publisherRedis(testRedisUrl())
        , publisher(publisherRedis)
        , router(redis, [collector = collector](const std::string& connectionId, const std::string& json)
              { (*collector)(connectionId, json); })
    {
        router.start();
        // The subscribe() call happens on the router's own background
        // thread -- give it a moment to actually register with Redis before
        // any TEST_CASE starts publishing, same as any other
        // eventually-consistent pub/sub setup would need.
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
};

Fixture& sharedFixture()
{
    static Fixture fixture;
    return fixture;
}
}

TEST_CASE("GameNodePushRouter: a \"push\" published by GameNodePushPublisher reaches the default handler")
{
    Fixture& fixture = sharedFixture();

    fixture.publisher.push("test-router-conn-1", R"({"type":"game_view"})");

    REQUIRE(fixture.collector->waitForAny(std::chrono::seconds(3)));

    const auto pushes = fixture.collector->snapshot();
    const bool found = std::any_of(pushes.begin(), pushes.end(), [](const CapturedPush& push)
        { return push.connectionId == "test-router-conn-1" && push.json == R"({"type":"game_view"})"; });
    CHECK(found);
}

TEST_CASE("GameNodePushRouter: waitForReconnectCheckResult returns the payload once it arrives")
{
    Fixture& fixture = sharedFixture();

    const std::optional<std::string> result = fixture.router.waitForReconnectCheckResult(
        "test-router-conn-2",
        3000,
        [&] { fixture.publisher.pushReconnectCheckResult("test-router-conn-2", R"({"sessionId":"abc"})"); });

    REQUIRE(result.has_value());
    CHECK(*result == R"({"sessionId":"abc"})");
}

TEST_CASE("GameNodePushRouter: waitForReconnectCheckResult times out if nothing ever replies")
{
    Fixture& fixture = sharedFixture();

    const std::optional<std::string> result =
        fixture.router.waitForReconnectCheckResult("test-router-conn-3-nobody-replies", 300, [] {});

    CHECK_FALSE(result.has_value());
}
