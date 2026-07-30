#ifndef REDIS_SHARD_LOAD_STORE_H
#define REDIS_SHARD_LOAD_STORE_H

#include <sw/redis++/redis++.h>

#include "IShardLoadStore.h"

// Redis-backed IShardLoadStore. One Redis Hash "shard_load", field =
// shardId, value = live session count -- a Hash (not one key per shard) so
// knownShards() is a single HKEYS instead of a SCAN/KEYS over the keyspace,
// same reasoning RedisSessionIndexStore's reverse-index set already used
// for a different cleanup problem. heartbeat()/isAlive() (MIGRATION_PLAN.md
// Phase 4c) use a separate key per shard, "shard_heartbeat:{shardId}", with
// a real TTL (GameNodeConfig::SHARD_HEARTBEAT_TTL_MILLIS) -- a crashed
// shard's own key simply expires with no explicit cleanup, which is exactly
// the point: this store can tell "known but not heartbeating" apart from
// "known and alive" without anything having to notice the crash happen.
class RedisShardLoadStore : public IShardLoadStore
{
public:
    explicit RedisShardLoadStore(sw::redis::Redis& redis);

    void registerShard(const std::string& shardId) override;
    void incrementLoad(const std::string& shardId) override;
    void decrementLoad(const std::string& shardId) override;
    std::vector<std::string> knownShards() const override;
    long long loadOf(const std::string& shardId) const override;
    void heartbeat(const std::string& shardId) override;
    bool isAlive(const std::string& shardId) const override;
    void forget(const std::string& shardId) override;

private:
    sw::redis::Redis& redis;
};

#endif
