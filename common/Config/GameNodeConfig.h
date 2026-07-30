#ifndef COMMON_CONFIG_GAME_NODE_CONFIG_H
#define COMMON_CONFIG_GAME_NODE_CONFIG_H

#include <string>

// Single source of truth for the Gateway<->Game Allocator<->Game Server Shard
// boundary introduced in MIGRATION_PLAN.md Phase 3 and reshaped by Phase 4b
// -- shared by server/ (the WebSocket Gateway), gameallocator/, and gamenode/
// (now a Game Server Shard), same pattern as TimingConfig.h/
// MatchmakingConfig.h. All processes must agree on these channel names since
// they're just ends of the same Redis pub/sub pipes (KUNGFUCHESS_REDIS_URL,
// mandatory for a real split -- see ARCHITECTURE.md's Known gaps).
namespace GameNodeConfig
{
    // Gateway -> Game Allocator: find_game requests and connection-closed
    // notifications (the Allocator's Matchmaker needs to know about a
    // dropped connection even if it was only ever queued, never matched) --
    // see GameNodeBridge/GameNodeMessages.h for the envelope shape. Exactly
    // one Allocator ever runs (MIGRATION_PLAN.md Phase 4's "one true
    // singleton in the topology"), so unlike the per-shard channel below,
    // this one is a single well-known name, not parameterized.
    inline constexpr const char* MATCHMAKING_REQUESTS_CHANNEL = "gameallocator:requests";

    // Gateway/Game Allocator -> a specific Game Server Shard: forwarded
    // move/jump requests, connection-closed notifications, reconnect-check
    // requests, and create-session requests (MIGRATION_PLAN.md Phase 4b --
    // replaces Phase 3's single, unscoped REQUESTS_CHANNEL, which only
    // worked because exactly one Game Node ever ran). Every reader/writer of
    // this channel must agree on the same shardId string a Game Server
    // Shard registers itself under (see IShardLoadStore::registerShard,
    // gamenode/src/main.cpp's KUNGFUCHESS_SHARD_ID).
    inline std::string shardRequestsChannel(const std::string& shardId)
    {
        return "gamenode:requests:" + shardId;
    }

    // Game Allocator/Game Server Shard -> Gateway: every push destined for a
    // specific connectionId (game_view/match_found/no_match/game_over/
    // opponent_disconnected/opponent_reconnected, and reconnect-check
    // replies). Deliberately still one shared channel, not per-shard --
    // every push already names its own connectionId, so a Gateway (there
    // may be more than one) just ignores whatever it doesn't hold, the same
    // tolerance a broadcast channel already needed before any of this
    // existed. Fan-out cost of that broadcast at real scale is a Phase 5
    // observability/tuning concern, not a correctness one.
    inline constexpr const char* PUSHES_CHANNEL = "gateway:pushes";

    // How long AuthRequestHandler's login path blocks waiting for a Game
    // Server Shard's answer to "does this user already have an active
    // session" before giving up and treating it as "no session found" --
    // this is the one place Phase 3 keeps a synchronous round trip (see
    // RemoteReconnectResolver), since CliShell relies on the resume push
    // arriving before login_result. Phase 4b: skipped entirely (no round
    // trip at all) when IGameShardRoutingStore has no entry for this userId,
    // since that already means "no active session" without needing to ask
    // any shard.
    constexpr long long RECONNECT_CHECK_TIMEOUT_MILLIS = 3000;

    // MIGRATION_PLAN.md Phase 4c (forfeit-on-crash): how often a Game Server
    // Shard refreshes its own liveness heartbeat (IShardLoadStore::heartbeat,
    // called from gamenode/'s tick loop) and how long the Game Allocator's
    // periodic health check (ShardHealthMonitor::checkAndForfeitDeadShards)
    // lets a heartbeat go stale before treating a shard as dead. TTL is a few
    // multiples of the refresh interval on purpose -- a single missed refresh
    // (a slow tick, a transient Redis hiccup) shouldn't false-positive a
    // perfectly live shard as dead and forfeit real, in-progress games.
    constexpr long long SHARD_HEARTBEAT_INTERVAL_MILLIS = 2000;
    constexpr long long SHARD_HEARTBEAT_TTL_MILLIS = 6000;
}

#endif
