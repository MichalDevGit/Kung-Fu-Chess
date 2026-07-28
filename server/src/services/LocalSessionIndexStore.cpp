#include "LocalSessionIndexStore.h"

void LocalSessionIndexStore::bindSession(
    const std::string& sessionId, const std::vector<std::string>& connectionIds, const std::vector<int>& userIds)
{
    std::lock_guard<std::mutex> lock(mutex);

    for (const std::string& connectionId : connectionIds)
        sessionIdByConnection[connectionId] = sessionId;

    for (int userId : userIds)
        sessionIdByUserId[userId] = sessionId;
}

void LocalSessionIndexStore::bindConnection(const std::string& connectionId, const std::string& sessionId)
{
    std::lock_guard<std::mutex> lock(mutex);
    sessionIdByConnection[connectionId] = sessionId;
}

void LocalSessionIndexStore::unbindConnection(const std::string& connectionId)
{
    std::lock_guard<std::mutex> lock(mutex);
    sessionIdByConnection.erase(connectionId);
}

void LocalSessionIndexStore::unbindSession(const std::string& sessionId)
{
    std::lock_guard<std::mutex> lock(mutex);

    for (auto connIt = sessionIdByConnection.begin(); connIt != sessionIdByConnection.end();)
        connIt = (connIt->second == sessionId) ? sessionIdByConnection.erase(connIt) : std::next(connIt);

    for (auto userIt = sessionIdByUserId.begin(); userIt != sessionIdByUserId.end();)
        userIt = (userIt->second == sessionId) ? sessionIdByUserId.erase(userIt) : std::next(userIt);
}

std::optional<std::string> LocalSessionIndexStore::findSessionIdByConnection(const std::string& connectionId) const
{
    std::lock_guard<std::mutex> lock(mutex);

    const auto it = sessionIdByConnection.find(connectionId);
    if (it == sessionIdByConnection.end())
        return std::nullopt;

    return it->second;
}

std::optional<std::string> LocalSessionIndexStore::findSessionIdByUserId(int userId) const
{
    std::lock_guard<std::mutex> lock(mutex);

    const auto it = sessionIdByUserId.find(userId);
    if (it == sessionIdByUserId.end())
        return std::nullopt;

    return it->second;
}
