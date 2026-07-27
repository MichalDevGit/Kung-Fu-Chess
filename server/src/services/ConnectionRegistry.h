#ifndef CONNECTION_REGISTRY_H
#define CONNECTION_REGISTRY_H

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

// Remembers which authenticated user a live WebSocket connection belongs to.
// Populated once, by AuthRequestHandler, the moment a `login` succeeds --
// every later request on that same connection (find_game/move/jump) is
// authorized against this binding instead of trusting a client-claimed
// identity in the request body.
//
// Kept as its own tiny service (mutex-guarded, no networking types) rather
// than folded into AuthService or GameSessionManager, since it's the one
// piece of state both matchmaking and gameplay need to answer "who is this
// connection" -- register() doesn't touch it at all, only login().
class ConnectionRegistry
{
public:
    struct AuthenticatedUser
    {
        int userId = 0;
        std::string username;
        int score = 0;
    };

    // Binds connectionId to user, replacing any previous binding for either
    // key (a user logging in again on a new connection supersedes their old
    // one -- see GameSessionManager::rebindConnection for what that means
    // for an in-progress game).
    void onAuthenticated(const std::string& connectionId, const AuthenticatedUser& user);

    // Called from WebSocketServer's close handler. Erases both directions of
    // the mapping for this connection.
    void onDisconnected(const std::string& connectionId);

    std::optional<AuthenticatedUser> find(const std::string& connectionId) const;

    // The most recently authenticated connection for this user, if any is
    // still bound -- lets a fresh login recognize "this user already has a
    // live connection" (reconnect) before matchmaking ever sees them.
    std::optional<std::string> findConnectionForUser(int userId) const;

private:
    mutable std::mutex mutex;
    std::unordered_map<std::string, AuthenticatedUser> byConnection;
    std::unordered_map<int, std::string> connectionByUserId;
};

#endif
