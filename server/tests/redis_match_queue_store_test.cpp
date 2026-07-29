// Requires a real, reachable Redis -- see redis_connection_store_test.cpp
// for why this isn't behind a compile-time macro.
#include "tests/doctest.h"
#include "services/Matchmaking/Matchmaker.h"
#include "services/Matchmaking/RedisMatchQueueStore.h"
#include "common/Config/MatchmakingConfig.h"

#include <cstdlib>
#include <sw/redis++/redis++.h>

namespace
{
    std::string testRedisUrl()
    {
        const char* url = std::getenv("KUNGFUCHESS_TEST_REDIS_URL");
        return url ? std::string(url) : "tcp://127.0.0.1:6379";
    }

    void resetQueue(sw::redis::Redis& redis)
    {
        redis.del("matchqueue");
    }
}

TEST_CASE("Matchmaker(RedisMatchQueueStore) pairs two queued entries within the score range, earliest-queued first")
{
    sw::redis::Redis redis(testRedisUrl());
    resetQueue(redis);
    RedisMatchQueueStore store(redis);
    Matchmaker matchmaker(store);

    matchmaker.enqueue(Matchmaker::Entry{"conn-a", 1, "alice", 1000, 100});
    matchmaker.enqueue(Matchmaker::Entry{"conn-b", 2, "bob", 1050, 200});

    std::vector<Matchmaker::Match> matches;
    matchmaker.tick(
        300,
        [&](const Matchmaker::Match& m) { matches.push_back(m); },
        [](const Matchmaker::Entry&) {});

    REQUIRE(matches.size() == 1);
    CHECK(matches[0].first.connectionId == "conn-a");
    CHECK(matches[0].second.connectionId == "conn-b");
    CHECK(matchmaker.isQueued("conn-a") == false);
    CHECK(matchmaker.isQueued("conn-b") == false);

    resetQueue(redis);
}

TEST_CASE("Matchmaker(RedisMatchQueueStore)::enqueue is idempotent per connection")
{
    sw::redis::Redis redis(testRedisUrl());
    resetQueue(redis);
    RedisMatchQueueStore store(redis);
    Matchmaker matchmaker(store);

    matchmaker.enqueue(Matchmaker::Entry{"conn-a", 1, "alice", 1000, 0});
    matchmaker.enqueue(Matchmaker::Entry{"conn-a", 1, "alice", 1000, 999});

    matchmaker.removeByConnection("conn-a");
    CHECK(matchmaker.isQueued("conn-a") == false);

    resetQueue(redis);
}

TEST_CASE("Matchmaker(RedisMatchQueueStore) times out an entry that has waited longer than MAX_WAIT_MILLIS")
{
    sw::redis::Redis redis(testRedisUrl());
    resetQueue(redis);
    RedisMatchQueueStore store(redis);
    Matchmaker matchmaker(store);

    matchmaker.enqueue(Matchmaker::Entry{"conn-a", 1, "alice", 1000, 0});

    std::vector<Matchmaker::Entry> timedOut;
    matchmaker.tick(
        MatchmakingConfig::MAX_WAIT_MILLIS + 1,
        [](const Matchmaker::Match&) {},
        [&](const Matchmaker::Entry& e) { timedOut.push_back(e); });

    REQUIRE(timedOut.size() == 1);
    CHECK(timedOut[0].connectionId == "conn-a");
    CHECK(matchmaker.isQueued("conn-a") == false);
}
