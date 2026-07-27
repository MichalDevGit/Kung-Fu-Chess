#ifndef AUTH_REQUEST_HANDLER_H
#define AUTH_REQUEST_HANDLER_H

#include <functional>
#include <string>

class AuthService;
class ConnectionRegistry;
class GameSessionManager;

// Translates a raw JSON request string into an AuthService call and a raw
// JSON response string. Knows the protocol/ message shapes; knows nothing
// about ixwebsocket or sockets, so it's usable and testable without a real
// connection -- WebSocketServer is the only thing that calls it.
//
// On a successful `login`, also records connectionId -> user in
// ConnectionRegistry -- this is the one place that binding is created, so
// every later request on this connection (find_game/move/jump) can be
// authorized against it instead of trusting client-claimed identity.
// `register` deliberately does not authenticate the connection.
//
// A login also checks whether this userId already has an active
// GameSession (a reconnect, e.g. the same player's connection dropped
// mid-game and they're logging back in) -- if so it rebinds that session to
// the new connection and pushes a MatchFoundResult-shaped resume through
// sendResume, entirely bypassing matchmaking. This is why a client can
// always auto-send find_game right after login (see client/src/main.cpp):
// by the time that request arrives, a returning player is already back in
// their game, not queued for a new one.
class AuthRequestHandler
{
public:
    using SendFn = std::function<void(const std::string& connectionId, const std::string& json)>;

    AuthRequestHandler(AuthService& authService, ConnectionRegistry& connectionRegistry, GameSessionManager& sessionManager, SendFn sendResume);

    // Never throws -- any parse/validation failure is caught internally and
    // turned into a protocol::ErrorResult envelope instead.
    std::string handle(const std::string& connectionId, const std::string& rawJson) const;

private:
    AuthService& authService;
    ConnectionRegistry& connectionRegistry;
    GameSessionManager& sessionManager;
    SendFn sendResume;
};

#endif
