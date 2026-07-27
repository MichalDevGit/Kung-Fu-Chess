#include "tests/doctest.h"
#include "services/Matchmaker.h"
#include "common/Config/MatchmakingConfig.h"

#include <vector>

TEST_CASE("Matchmaker pairs two queued entries within the score range, earliest-queued first")
{
    Matchmaker matchmaker;
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
}

TEST_CASE("Matchmaker does not pair entries whose scores are too far apart")
{
    Matchmaker matchmaker;
    matchmaker.enqueue(Matchmaker::Entry{"conn-a", 1, "alice", 1000, 100});
    matchmaker.enqueue(Matchmaker::Entry{"conn-b", 2, "bob", 1000 + MatchmakingConfig::SCORE_RANGE + 1, 200});

    std::vector<Matchmaker::Match> matches;
    matchmaker.tick(
        300,
        [&](const Matchmaker::Match& m) { matches.push_back(m); },
        [](const Matchmaker::Entry&) {});

    CHECK(matches.empty());
    CHECK(matchmaker.isQueued("conn-a") == true);
    CHECK(matchmaker.isQueued("conn-b") == true);
}

TEST_CASE("Matchmaker times out an entry that has waited longer than MAX_WAIT_MILLIS with no match")
{
    Matchmaker matchmaker;
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

TEST_CASE("Matchmaker::enqueue is idempotent per connection")
{
    Matchmaker matchmaker;
    matchmaker.enqueue(Matchmaker::Entry{"conn-a", 1, "alice", 1000, 0});
    matchmaker.enqueue(Matchmaker::Entry{"conn-a", 1, "alice", 1000, 999});

    matchmaker.removeByConnection("conn-a");
    CHECK(matchmaker.isQueued("conn-a") == false);
}
