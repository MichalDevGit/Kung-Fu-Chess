#include "ConnectionRegistry.h"

ConnectionRegistry::ConnectionRegistry(IConnectionStore& store) : store(store)
{
}

void ConnectionRegistry::onAuthenticated(const std::string& connectionId, const AuthenticatedUser& user)
{
    std::lock_guard<std::mutex> lock(mutex);

    // A user reconnecting on a new connection supersedes whichever
    // connection they were previously bound to -- drop that stale forward
    // mapping so it doesn't linger pointing at a connection id that may
    // already be closed.
    const std::optional<std::string> existing = store.findConnectionForUser(user.userId);
    if (existing.has_value() && *existing != connectionId)
        store.erase(*existing);

    store.set(connectionId, user);
    store.setUserConnection(user.userId, connectionId);
}

void ConnectionRegistry::onDisconnected(const std::string& connectionId)
{
    std::lock_guard<std::mutex> lock(mutex);

    const std::optional<AuthenticatedUser> user = store.find(connectionId);
    if (!user.has_value())
        return;

    // Only erase the reverse mapping if it still points at this connection
    // -- onAuthenticated may have already redirected it to a newer
    // connection for the same user by the time this close arrives.
    const std::optional<std::string> reverse = store.findConnectionForUser(user->userId);
    if (reverse.has_value() && *reverse == connectionId)
        store.eraseUserConnection(user->userId);

    store.erase(connectionId);
}

std::optional<ConnectionRegistry::AuthenticatedUser> ConnectionRegistry::find(const std::string& connectionId) const
{
    std::lock_guard<std::mutex> lock(mutex);
    return store.find(connectionId);
}

std::optional<std::string> ConnectionRegistry::findConnectionForUser(int userId) const
{
    std::lock_guard<std::mutex> lock(mutex);
    return store.findConnectionForUser(userId);
}
