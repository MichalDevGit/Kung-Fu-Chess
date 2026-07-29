#ifndef AUTH_REQUEST_HANDLER_H
#define AUTH_REQUEST_HANDLER_H

#include <functional>
#include <string>

class ConnectionRegistry;
class IReconnectResolver;
class IUserRepository;
namespace security
{
    class TokenService;
}

// Translates a raw JSON request string into a raw JSON response string.
// Knows the protocol/ message shapes; knows nothing about ixwebsocket or
// sockets, so it's usable and testable without a real connection --
// WebSocketServer is the only thing that calls it.
//
// MIGRATION_PLAN.md Phase 2, step B: `register`/password-based `login` are
// gone from this process entirely -- they now live in the separate API
// Gateway process (apigateway/), which owns AuthService/PasswordHasher.
// This handler only ever verifies a `login_token` (a signed token the
// gateway already issued after checking the password itself) via
// TokenService, then looks the user up by id via IUserRepository purely to
// read their username/score -- it never sees, stores, or compares a
// password. This is what makes "the WebSocket process no longer touches
// passwords directly" concretely true: KungFuChessAuth/KungFuChessSecurity
// (bcrypt) are no longer linked into KungFuChessServer at all (see
// server/CMakeLists.txt).
//
// On a successful login_token, records connectionId -> user in
// ConnectionRegistry -- this is the one place that binding is created, so
// every later request on this connection (find_game/move/jump) can be
// authorized against it instead of trusting client-claimed identity.
//
// A login also checks whether this userId already has an active
// GameSession (a reconnect, e.g. the same player's connection dropped
// mid-game and they're logging back in) -- if so it rebinds that session to
// the new connection and pushes a MatchFoundResult-shaped resume through
// sendResume, entirely bypassing matchmaking. This is why a client can
// always auto-send find_game right after login (see client/src/main.cpp):
// by the time that request arrives, a returning player is already back in
// their game, not queued for a new one.
//
// MIGRATION_PLAN.md Phase 3: that reconnect check is behind IReconnectResolver
// now, not a direct GameSessionManager& -- GameSessionManager moved into its
// own process (gamenode/) in this phase, so the WebSocket Gateway (where this
// class still lives) can only ask the question over the network
// (RemoteReconnectResolver). LocalReconnectResolver (a direct in-process
// call) is what tests -- and gamenode/'s own request router -- use instead.
class AuthRequestHandler
{
public:
    using SendFn = std::function<void(const std::string& connectionId, const std::string& json)>;

    AuthRequestHandler(
        IUserRepository& users,
        security::TokenService& tokenService,
        ConnectionRegistry& connectionRegistry,
        IReconnectResolver& reconnectResolver,
        SendFn sendResume);

    // Never throws -- any parse/validation failure is caught internally and
    // turned into a protocol::ErrorResult envelope instead.
    std::string handle(const std::string& connectionId, const std::string& rawJson) const;

private:
    IUserRepository& users;
    security::TokenService& tokenService;
    ConnectionRegistry& connectionRegistry;
    IReconnectResolver& reconnectResolver;
    SendFn sendResume;

    // Binds the connection's identity and resumes an existing session if
    // one is found. Returns the login_result JSON to reply with.
    std::string completeLogin(const std::string& connectionId, int userId, const std::string& username, int score) const;
};

#endif
