#include "tests/doctest.h"
#include "services/Sharding/GameAllocator.h"
#include "services/Sharding/LocalGameShardRoutingStore.h"
#include "services/Sharding/LocalShardLoadStore.h"
#include "services/GameSession/GameSession.h"

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace
{
Match testMatch()
{
    return Match{Entry{"conn-a", 1, "alice", 1000, 0}, Entry{"conn-b", 2, "bob", 1000, 100}};
}

struct RequestedSessionCreation
{
    std::string sessionId;
    GameSession::Player white;
    GameSession::Player black;
    std::string shardId;
};

GameAllocator::RequestSessionCreationFn recordingRequestSessionCreation(std::vector<RequestedSessionCreation>& requests)
{
    return [&requests](const std::string& sessionId, GameSession::Player white, GameSession::Player black, const std::string& shardId)
    { requests.push_back(RequestedSessionCreation{sessionId, std::move(white), std::move(black), shardId}); };
}
}

TEST_CASE("GameAllocator::allocate records routing for the session and both participants on the only registered shard")
{
    LocalShardLoadStore loadStore;
    LocalGameShardRoutingStore routingStore;
    std::vector<RequestedSessionCreation> requests;

    loadStore.registerShard("shard-1");
    GameAllocator allocator(loadStore, routingStore, recordingRequestSessionCreation(requests));

    const GameAllocator::Allocation allocation = allocator.allocate(testMatch());

    REQUIRE(requests.size() == 1);
    CHECK(requests[0].sessionId == allocation.sessionId);
    CHECK(requests[0].shardId == "shard-1");
    CHECK(requests[0].white.userId == 1);
    CHECK(requests[0].black.userId == 2);

    CHECK(*routingStore.findShardForSession(allocation.sessionId) == "shard-1");
    CHECK(*routingStore.findShardForUser(1) == "shard-1");
    CHECK(*routingStore.findShardForUser(2) == "shard-1");
    CHECK(loadStore.loadOf("shard-1") == 1);

    // MIGRATION_PLAN.md Phase 4c: ShardHealthMonitor needs both of these to
    // enumerate a dead shard's sessions and each one's participants.
    const std::vector<std::string> sessions = routingStore.sessionsForShard("shard-1");
    REQUIRE(sessions.size() == 1);
    CHECK(sessions[0] == allocation.sessionId);

    const std::vector<int> users = routingStore.usersForSession(allocation.sessionId);
    CHECK(std::count(users.begin(), users.end(), 1) == 1);
    CHECK(std::count(users.begin(), users.end(), 2) == 1);
}

TEST_CASE("GameAllocator::allocate returns the deterministic classic starting position as the initial view")
{
    LocalShardLoadStore loadStore;
    LocalGameShardRoutingStore routingStore;
    std::vector<RequestedSessionCreation> requests;

    loadStore.registerShard("shard-1");
    GameAllocator allocator(loadStore, routingStore, recordingRequestSessionCreation(requests));

    const GameAllocator::Allocation allocation = allocator.allocate(testMatch());

    // BoardView is a dense 8x8 grid (64 slots, empty squares included -- see
    // ARCHITECTURE.md's Gaps #2); 32 of those are actually occupied on a
    // fresh classic board, no motion/jump in flight, no rests yet, no
    // selection, clock at 0 -- see GameFactory::createClassicBoard.
    const auto& pieces = allocation.initialView.getBoard().getPieces();
    CHECK(pieces.size() == 64);
    CHECK(std::count_if(pieces.begin(), pieces.end(), [](const PieceView& p) { return !p.isEmpty(); }) == 32);
    CHECK(allocation.initialView.getCurrentTime() == 0);
    CHECK(allocation.initialView.getHasSelection() == false);
}

TEST_CASE("GameAllocator::allocate picks the least-loaded of several registered shards")
{
    LocalShardLoadStore loadStore;
    LocalGameShardRoutingStore routingStore;
    std::vector<RequestedSessionCreation> requests;

    loadStore.registerShard("shard-1");
    loadStore.registerShard("shard-2");
    loadStore.incrementLoad("shard-1"); // shard-1 already hosts one session

    GameAllocator allocator(loadStore, routingStore, recordingRequestSessionCreation(requests));
    const GameAllocator::Allocation allocation = allocator.allocate(testMatch());

    CHECK(*routingStore.findShardForSession(allocation.sessionId) == "shard-2");
    CHECK(loadStore.loadOf("shard-2") == 1);
    CHECK(loadStore.loadOf("shard-1") == 1); // unchanged
}

TEST_CASE("GameAllocator::allocate throws if no shard is registered")
{
    LocalShardLoadStore loadStore;
    LocalGameShardRoutingStore routingStore;
    std::vector<RequestedSessionCreation> requests;

    GameAllocator allocator(loadStore, routingStore, recordingRequestSessionCreation(requests));

    CHECK_THROWS_AS(allocator.allocate(testMatch()), std::runtime_error);
}
