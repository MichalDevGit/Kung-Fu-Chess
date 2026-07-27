#include "ConnectionRegistry.h"

void ConnectionRegistry::onAuthenticated(const std::string& connectionId, const AuthenticatedUser& user)
{
    std::lock_guard<std::mutex> lock(mutex);

    // A user reconnecting on a new connection supersedes whichever
    // connection they were previously bound to -- drop that stale forward
    // mapping so it doesn't linger pointing at a connection id that may
    // already be closed.
    const auto existing = connectionByUserId.find(user.userId);
    if (existing != connectionByUserId.end() && existing->second != connectionId)
        byConnection.erase(existing->second);

    byConnection[connectionId] = user;
    connectionByUserId[user.userId] = connectionId;
}

void ConnectionRegistry::onDisconnected(const std::string& connectionId)
{
    std::lock_guard<std::mutex> lock(mutex);

    const auto it = byConnection.find(connectionId);
    if (it == byConnection.end())
        return;

    // Only erase the reverse mapping if it still points at this connection
    // -- onAuthenticated may have already redirected it to a newer
    // connection for the same user by the time this close arrives.
    const auto reverse = connectionByUserId.find(it->second.userId);
    if (reverse != connectionByUserId.end() && reverse->second == connectionId)
        connectionByUserId.erase(reverse);

    byConnection.erase(it);
}

std::optional<ConnectionRegistry::AuthenticatedUser> ConnectionRegistry::find(const std::string& connectionId) const
{
    std::lock_guard<std::mutex> lock(mutex);

    const auto it = byConnection.find(connectionId);
    if (it == byConnection.end())
        return std::nullopt;

    return it->second;
}

std::optional<std::string> ConnectionRegistry::findConnectionForUser(int userId) const
{
    std::lock_guard<std::mutex> lock(mutex);

    const auto it = connectionByUserId.find(userId);
    if (it == connectionByUserId.end())
        return std::nullopt;

    return it->second;
}
