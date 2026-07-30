#ifndef SHARD_HEALTH_MONITOR_H
#define SHARD_HEALTH_MONITOR_H

#include <functional>
#include <optional>
#include <string>

class IShardLoadStore;
class IGameShardRoutingStore;

// MIGRATION_PLAN.md Phase 4c's forfeit-on-crash: detects a Game Server
// Shard whose heartbeat (IShardLoadStore::isAlive) has lapsed and, for
// every session IGameShardRoutingStore says it was hosting, pushes a
// defined GameOverMessage to both participants instead of leaving them
// silently stuck on a board that will never update again -- "either
// [forfeit or resume from a snapshot] is fine as long as it's a defined,
// tested behavior rather than silence" (MIGRATION_PLAN.md Phase 4's exit
// criteria). Snapshot-resume is a deliberately separate, larger follow-up;
// this is the smaller, first-shipped half.
//
// Deliberately applies no rating change: a shard crash is nobody's fault,
// so unlike GameSession::forfeitTo's disconnect-timeout forfeit (which does
// update ELO, since a real opponent stuck around and "won"), this pushes
// GameOverMessage with no winnerUserId at all. That's also why this class
// needs no IUserRepository/RatingService -- it only ever pushes a JSON
// message to whichever connection a still-registered participant happens
// to be on right now (resolved fresh via findConnectionForUser, never
// cached, since a participant may have reconnected on a new connection id
// since the session was created), never touches persistence.
class ShardHealthMonitor
{
public:
    using FindConnectionForUserFn = std::function<std::optional<std::string>(int userId)>;
    using PushFn = std::function<void(const std::string& connectionId, const std::string& json)>;

    ShardHealthMonitor(
        IShardLoadStore& loadStore,
        IGameShardRoutingStore& routingStore,
        FindConnectionForUserFn findConnectionForUser,
        PushFn push);

    // Checks every currently-registered shard; any whose heartbeat has
    // lapsed has every session it was hosting forfeited (both participants
    // pushed a GameOverMessage, if still resolvable to a live connection --
    // a participant who simply isn't connected right now just doesn't get
    // one, same as any other push this codebase already tolerates missing)
    // and is then forgotten entirely (IShardLoadStore::forget), so it's
    // never picked again by GameAllocator and this check doesn't keep
    // re-firing for it on the next call.
    void checkAndForfeitDeadShards();

private:
    IShardLoadStore& loadStore;
    IGameShardRoutingStore& routingStore;
    FindConnectionForUserFn findConnectionForUser;
    PushFn push;

    void forfeitShard(const std::string& shardId);
};

#endif
