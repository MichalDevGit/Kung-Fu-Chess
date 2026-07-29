#ifndef CONNECTION_REGISTRY_H
#define CONNECTION_REGISTRY_H

#include <mutex>
#include <optional>
#include <string>

#include "AuthenticatedUser.h"
#include "IConnectionStore.h"

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
//
// Raw storage lives behind IConnectionStore (LocalConnectionStore by
// default, RedisConnectionStore for a multi-node deployment -- see
// server/src/main.cpp's KUNGFUCHESS_REDIS_URL wiring); this class's own
// mutex is what keeps the multi-step "read existing, conditionally erase,
// then set" sequences in onAuthenticated/onDisconnected atomic, which is
// necessary and sufficient as long as this remains one process -- true
// cross-process atomicity (Lua scripts / WATCH+MULTI) is out of scope until
// a later phase actually splits into multiple processes sharing one Redis.
class ConnectionRegistry
{
public:
    using AuthenticatedUser = ::AuthenticatedUser;

    explicit ConnectionRegistry(IConnectionStore& store);

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
    IConnectionStore& store;
};

#endif
