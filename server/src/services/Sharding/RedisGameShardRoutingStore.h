#ifndef REDIS_GAME_SHARD_ROUTING_STORE_H
#define REDIS_GAME_SHARD_ROUTING_STORE_H

#include <sw/redis++/redis++.h>

#include "IGameShardRoutingStore.h"

// Redis-backed IGameShardRoutingStore. Keys: "shard_session:{sessionId}" /
// "shard_user:{userId}" -> shardId, plus an auxiliary Redis Set
// "shard_keys:{sessionId}" (members = the exact Redis keys created for that
// session) so unbindSession can clean up in O(session size) via
// SMEMBERS+DEL instead of a SCAN/KEYS over the whole keyspace -- same
// reverse-index bookkeeping RedisSessionIndexStore already uses, for the
// same reason. MIGRATION_PLAN.md Phase 4c adds two more keys:
// "shard_sessions:{shardId}" (a Set of every sessionId currently routed to
// that shard -- NOT cleaned up via the shard_keys reverse-index above,
// since it's shared across every session on that shard, not unique per
// session; unbindSession removes just this session's own membership via
// SREM instead of blanket-deleting it) and "session_users:{sessionId}" (a
// Set of that session's participant userIds, unique per session so it *can*
// go through the normal shard_keys-tracked cleanup).
class RedisGameShardRoutingStore : public IGameShardRoutingStore
{
public:
    explicit RedisGameShardRoutingStore(sw::redis::Redis& redis);

    void bindSession(const std::string& sessionId, const std::vector<int>& userIds, const std::string& shardId) override;
    void unbindSession(const std::string& sessionId, const std::vector<int>& userIds) override;
    std::optional<std::string> findShardForSession(const std::string& sessionId) const override;
    std::optional<std::string> findShardForUser(int userId) const override;
    std::vector<std::string> sessionsForShard(const std::string& shardId) const override;
    std::vector<int> usersForSession(const std::string& sessionId) const override;

private:
    sw::redis::Redis& redis;

    void trackKeyForSession(const std::string& sessionId, const std::string& key);
};

#endif
