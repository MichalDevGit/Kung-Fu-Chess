#ifndef GAME_ALLOCATOR_H
#define GAME_ALLOCATOR_H

#include <functional>
#include <mutex>
#include <string>

#include "../GameSession/GameSession.h"
#include "../Matchmaking/MatchmakingTypes.h"
#include "../../../../common/DTO/GameView.h"

class IGameShardRoutingStore;
class IShardLoadStore;

// MIGRATION_PLAN.md Phase 4's Game Allocator: on a Matchmaker pairing,
// decides which Game Server Shard hosts the new session and records that
// decision.
//
// Phase 4b: this class now lives in its own gameallocator/ process, entirely
// separate from any Game Server Shard's GameSessionManager -- so unlike
// Phase 4a's design (an in-process createSession callback returning a real
// GameSession&), allocate() can no longer read a live session's id/GameView
// back from a synchronous call. Two things make this unnecessary rather than
// requiring a cross-process round trip:
//  - sessionId is assigned *here*, by this class, and simply handed to
//    requestSessionCreation -- the shard is told what id to use, it doesn't
//    invent one GameSessionManager::createSession takes it as a parameter
//    now for exactly this reason).
//  - A freshly created session's initial GameView is always the same
//    deterministic classic starting position (GameFactory::
//    createClassicBoard assigns fixed piece ids from 0 every time; a fresh
//    RealTimeArbiter's clock always starts at 0) -- so this class computes
//    it locally (see initialGameView() in the .cpp) instead of waiting for
//    whichever shard actually ends up hosting the session to compute and
//    report back the identical thing.
// requestSessionCreation is therefore genuinely fire-and-forget: in
// production it publishes a GameNodeCreateSessionRequest to the picked
// shard's channel and returns immediately.
class GameAllocator
{
public:
    using RequestSessionCreationFn = std::function<void(
        const std::string& sessionId, GameSession::Player white, GameSession::Player black, const std::string& shardId)>;

    struct Allocation
    {
        std::string sessionId;
        GameView initialView;
    };

    GameAllocator(IShardLoadStore& loadStore, IGameShardRoutingStore& routingStore, RequestSessionCreationFn requestSessionCreation);

    // Picks the least-loaded currently-registered shard (ties broken by
    // whichever IShardLoadStore::knownShards() happens to list first),
    // assigns a new session id, fires off the (fire-and-forget) session
    // creation request, records the routing decision, and bumps that
    // shard's load. Throws std::runtime_error if no shard is registered at
    // all (a startup-ordering bug, not a runtime condition a caller should
    // ever need to handle -- see gamenode/src/main.cpp, which registers its
    // own shard before starting the tick loop that can call this).
    Allocation allocate(const Match& match);

private:
    IShardLoadStore& loadStore;
    IGameShardRoutingStore& routingStore;
    RequestSessionCreationFn requestSessionCreation;

    std::mutex mutex;
    long long nextSessionNumber = 1;

    std::string pickShard() const;
    std::string nextSessionId();
};

#endif
