#ifndef IGAME_SHARD_ROUTING_STORE_H
#define IGAME_SHARD_ROUTING_STORE_H

#include <optional>
#include <string>
#include <vector>

// Where a live GameSession actually lives, once more than one Game Server
// Shard exists (MIGRATION_PLAN.md Phase 4). A Gateway (or, once Phase 4b
// splits GameAllocator into its own process, the Allocator itself) needs
// this to know which shard a move/jump/reconnect_check for a given
// session/user should be forwarded to -- GameNodeConfig::REQUESTS_CHANNEL's
// single, unscoped channel only ever worked because Phase 3 ran exactly one
// Game Node. Mirrors the IUserRepository/ISessionIndexStore pattern:
// LocalGameShardRoutingStore is the in-memory default (single-shard
// deployments, tests), RedisGameShardRoutingStore is the cross-process
// alternative a real multi-shard deployment needs, since the writer of a
// routing decision (GameAllocator) and its readers (a Gateway, or another
// Game Node process) are different processes.
//
// Deliberately a separate store from ISessionIndexStore: that one answers
// "which session is this connection/user in," entirely within one Game
// Node's own process; this one answers "which shard hosts that session,"
// which only becomes a meaningful question once more than one shard exists.
class IGameShardRoutingStore
{
public:
    virtual ~IGameShardRoutingStore() = default;

    // Records that sessionId (and both its participants' userIds) now live
    // on shardId -- called once, by GameAllocator, right after it picks a
    // shard for a freshly matched pair.
    virtual void bindSession(const std::string& sessionId, const std::vector<int>& userIds, const std::string& shardId) = 0;

    // Removes sessionId's (and its participants') routing entries -- called
    // once a session finishes, mirroring ISessionIndexStore::unbindSession.
    virtual void unbindSession(const std::string& sessionId, const std::vector<int>& userIds) = 0;

    virtual std::optional<std::string> findShardForSession(const std::string& sessionId) const = 0;
    virtual std::optional<std::string> findShardForUser(int userId) const = 0;

    // MIGRATION_PLAN.md Phase 4c (forfeit-on-crash): every sessionId
    // currently routed to shardId -- what ShardHealthMonitor enumerates once
    // it decides a shard is dead, so it knows which sessions to forfeit.
    virtual std::vector<std::string> sessionsForShard(const std::string& shardId) const = 0;

    // The userIds bindSession was given for sessionId -- what
    // ShardHealthMonitor needs to resolve each participant's current
    // connection (via ConnectionRegistry::findConnectionForUser) to push a
    // forfeit message to, since a stale connectionId captured at match time
    // could have been superseded by a reconnect since.
    virtual std::vector<int> usersForSession(const std::string& sessionId) const = 0;
};

#endif
