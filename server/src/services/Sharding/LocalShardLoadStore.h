#ifndef LOCAL_SHARD_LOAD_STORE_H
#define LOCAL_SHARD_LOAD_STORE_H

#include <mutex>
#include <unordered_map>

#include "IShardLoadStore.h"

// In-memory IShardLoadStore -- today's default (a single-process, single-
// shard deployment, or a test, never needs more than this). heartbeat()/
// isAlive()/forget() (MIGRATION_PLAN.md Phase 4c) are trivial here on
// purpose: there's no real cross-process shard-crash scenario to detect
// within a single process, so every registered shard just reports alive
// forever until explicitly forget()-ten. Tests that need to exercise
// ShardHealthMonitor's dead-shard path use a small hand-written
// IShardLoadStore test double instead of this class (see
// server/tests/shard_health_monitor_test.cpp), specifically because this
// class can't simulate a stale heartbeat.
class LocalShardLoadStore : public IShardLoadStore
{
public:
    void registerShard(const std::string& shardId) override;
    void incrementLoad(const std::string& shardId) override;
    void decrementLoad(const std::string& shardId) override;
    std::vector<std::string> knownShards() const override;
    long long loadOf(const std::string& shardId) const override;
    void heartbeat(const std::string& shardId) override;
    bool isAlive(const std::string& shardId) const override;
    void forget(const std::string& shardId) override;

private:
    mutable std::mutex mutex;
    std::unordered_map<std::string, long long> loadByShard;
};

#endif
