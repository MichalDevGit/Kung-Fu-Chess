#include "LocalGameShardRoutingStore.h"

void LocalGameShardRoutingStore::bindSession(
    const std::string& sessionId, const std::vector<int>& userIds, const std::string& shardId)
{
    std::lock_guard<std::mutex> lock(mutex);

    shardIdBySession[sessionId] = shardId;
    for (int userId : userIds)
        shardIdByUser[userId] = shardId;

    sessionIdsByShard[shardId].insert(sessionId);
    userIdsBySession[sessionId] = userIds;
}

void LocalGameShardRoutingStore::unbindSession(const std::string& sessionId, const std::vector<int>& userIds)
{
    std::lock_guard<std::mutex> lock(mutex);

    const auto shardIt = shardIdBySession.find(sessionId);
    if (shardIt != shardIdBySession.end())
    {
        const auto sessionsIt = sessionIdsByShard.find(shardIt->second);
        if (sessionsIt != sessionIdsByShard.end())
            sessionsIt->second.erase(sessionId);
    }

    shardIdBySession.erase(sessionId);
    userIdsBySession.erase(sessionId);
    for (int userId : userIds)
        shardIdByUser.erase(userId);
}

std::optional<std::string> LocalGameShardRoutingStore::findShardForSession(const std::string& sessionId) const
{
    std::lock_guard<std::mutex> lock(mutex);

    const auto it = shardIdBySession.find(sessionId);
    return it != shardIdBySession.end() ? std::optional<std::string>(it->second) : std::nullopt;
}

std::optional<std::string> LocalGameShardRoutingStore::findShardForUser(int userId) const
{
    std::lock_guard<std::mutex> lock(mutex);

    const auto it = shardIdByUser.find(userId);
    return it != shardIdByUser.end() ? std::optional<std::string>(it->second) : std::nullopt;
}

std::vector<std::string> LocalGameShardRoutingStore::sessionsForShard(const std::string& shardId) const
{
    std::lock_guard<std::mutex> lock(mutex);

    const auto it = sessionIdsByShard.find(shardId);
    if (it == sessionIdsByShard.end())
        return {};

    return std::vector<std::string>(it->second.begin(), it->second.end());
}

std::vector<int> LocalGameShardRoutingStore::usersForSession(const std::string& sessionId) const
{
    std::lock_guard<std::mutex> lock(mutex);

    const auto it = userIdsBySession.find(sessionId);
    return it != userIdsBySession.end() ? it->second : std::vector<int>();
}
