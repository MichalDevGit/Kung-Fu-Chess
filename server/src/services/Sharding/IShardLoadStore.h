#ifndef ISHARD_LOAD_STORE_H
#define ISHARD_LOAD_STORE_H

#include <string>
#include <vector>

// Tracks how many live sessions each known Game Server Shard currently
// hosts, so GameAllocator can pick the least-loaded shard instead of a
// blind round-robin once MIGRATION_PLAN.md Phase 4b actually runs more than
// one. LocalShardLoadStore is an in-memory default (correct for the
// single-shard step this interface is first introduced under, and for
// tests -- with only one registered shard, "least loaded" always resolves
// to it); RedisShardLoadStore is what a real multi-shard deployment needs,
// since every Gateway/Allocator process must agree on the same load counts.
class IShardLoadStore
{
public:
    virtual ~IShardLoadStore() = default;

    // Registers shardId with zero load if it isn't already known -- a Game
    // Server Shard calls this once at startup (see gamenode/src/main.cpp).
    // No-op if shardId is already registered.
    virtual void registerShard(const std::string& shardId) = 0;

    virtual void incrementLoad(const std::string& shardId) = 0;
    virtual void decrementLoad(const std::string& shardId) = 0;

    // Every currently-registered shard id, in no particular order.
    virtual std::vector<std::string> knownShards() const = 0;
    virtual long long loadOf(const std::string& shardId) const = 0;

    // MIGRATION_PLAN.md Phase 4c (forfeit-on-crash): refreshes shardId's
    // liveness -- a Game Server Shard calls this from its own tick loop
    // roughly every GameNodeConfig::SHARD_HEARTBEAT_INTERVAL_MILLIS.
    // RedisShardLoadStore backs this with a real TTL key, so a crashed
    // shard's heartbeat naturally expires with no explicit cleanup needed;
    // LocalShardLoadStore has no meaningful TTL model (there's no real
    // cross-process failure to detect within one process) and just treats
    // every registered shard as permanently alive -- see its own header
    // comment.
    virtual void heartbeat(const std::string& shardId) = 0;

    // False once shardId's heartbeat has gone stale past its TTL (or it was
    // never registered/heartbeaten at all) -- what ShardHealthMonitor polls
    // to decide a shard is dead.
    virtual bool isAlive(const std::string& shardId) const = 0;

    // Removes shardId from knownShards()/loadOf() and clears its heartbeat
    // entirely -- called once ShardHealthMonitor has forfeited every session
    // a dead shard was hosting, so this same shard id is never picked again
    // and this check doesn't keep re-firing for it.
    virtual void forget(const std::string& shardId) = 0;
};

#endif
