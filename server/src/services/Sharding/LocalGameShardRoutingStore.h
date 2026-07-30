#ifndef LOCAL_GAME_SHARD_ROUTING_STORE_H
#define LOCAL_GAME_SHARD_ROUTING_STORE_H

#include <mutex>
#include <unordered_map>
#include <unordered_set>

#include "IGameShardRoutingStore.h"

// In-memory IGameShardRoutingStore -- today's default (a single-process,
// single-shard deployment, or a test, never needs more than this). Mirrors
// LocalSessionIndexStore's shape exactly, just keyed by shardId instead of
// sessionId.
class LocalGameShardRoutingStore : public IGameShardRoutingStore
{
public:
    void bindSession(const std::string& sessionId, const std::vector<int>& userIds, const std::string& shardId) override;
    void unbindSession(const std::string& sessionId, const std::vector<int>& userIds) override;
    std::optional<std::string> findShardForSession(const std::string& sessionId) const override;
    std::optional<std::string> findShardForUser(int userId) const override;
    std::vector<std::string> sessionsForShard(const std::string& shardId) const override;
    std::vector<int> usersForSession(const std::string& sessionId) const override;

private:
    mutable std::mutex mutex;
    std::unordered_map<std::string, std::string> shardIdBySession;
    std::unordered_map<int, std::string> shardIdByUser;
    std::unordered_map<std::string, std::unordered_set<std::string>> sessionIdsByShard;
    std::unordered_map<std::string, std::vector<int>> userIdsBySession;
};

#endif
