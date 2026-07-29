#include "LocalConnectionStore.h"

void LocalConnectionStore::set(const std::string& connectionId, const AuthenticatedUser& user)
{
    std::lock_guard<std::mutex> lock(mutex);
    byConnection[connectionId] = user;
}

void LocalConnectionStore::erase(const std::string& connectionId)
{
    std::lock_guard<std::mutex> lock(mutex);
    byConnection.erase(connectionId);
}

std::optional<AuthenticatedUser> LocalConnectionStore::find(const std::string& connectionId) const
{
    std::lock_guard<std::mutex> lock(mutex);

    const auto it = byConnection.find(connectionId);
    if (it == byConnection.end())
        return std::nullopt;

    return it->second;
}

void LocalConnectionStore::setUserConnection(int userId, const std::string& connectionId)
{
    std::lock_guard<std::mutex> lock(mutex);
    connectionByUserId[userId] = connectionId;
}

void LocalConnectionStore::eraseUserConnection(int userId)
{
    std::lock_guard<std::mutex> lock(mutex);
    connectionByUserId.erase(userId);
}

std::optional<std::string> LocalConnectionStore::findConnectionForUser(int userId) const
{
    std::lock_guard<std::mutex> lock(mutex);

    const auto it = connectionByUserId.find(userId);
    if (it == connectionByUserId.end())
        return std::nullopt;

    return it->second;
}
