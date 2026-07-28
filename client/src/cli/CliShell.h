#ifndef CLI_SHELL_H
#define CLI_SHELL_H

#include <condition_variable>
#include <mutex>
#include <optional>
#include <string>

#include "protocol/Message.h"

class ApiGatewayClient;
class WebSocketClient;

// The interactive std::cin loop: parses "register <user> <pass>" /
// "login <user> <pass>" / "help" / "quit". Runs on the calling (main)
// thread.
//
// MIGRATION_PLAN.md Phase 2: `register`/`login` are now REST calls against
// ApiGatewayClient (a synchronous HTTP request/response -- `register` needs
// no WebSocket involvement at all now). A successful `login` then sends the
// token it got back over the WebSocket connection as a
// LoginWithTokenRequest -- that part still genuinely belongs on the socket,
// since it's what binds this connection's identity and lets the server
// check for a resumable session (see AuthRequestHandler).
//
// Installs itself as the WebSocketClient's message handler for the
// duration of run() (see WebSocketClient::setOnMessage) -- it's the only
// thing listening to the socket during the auth phase, both to print
// server responses and to recognize the login_result that follows the
// login_token send so `login` can block until it knows whether it actually
// succeeded, instead of firing the request and leaving the caller to guess.
// Once run() returns successfully, client/src/main.cpp installs GameClient
// as the next message handler for the game phase -- same deferred-handoff
// pattern WebSocketClient::setOnMessage was built for.
class CliShell
{
public:
    struct LoginOutcome
    {
        bool loggedIn = false;
        int userId = 0;
        std::string username;
        int score = 0;

        // Populated only for a reconnecting player: the server pushes a
        // MatchFoundResult-shaped resume as part of a successful login when
        // ConnectionRegistry/GameSessionManager recognize an existing
        // session for this user (see AuthRequestHandler), and it is always
        // delivered before the login_result reply itself on the same
        // connection -- so by the time login_result unblocks run() below,
        // this is already known one way or the other. When set, the caller
        // (client/src/main.cpp) should skip matchmaking entirely and go
        // straight to the game.
        std::optional<protocol::MatchFoundResult> resumedMatch;
    };

    CliShell(WebSocketClient& client, ApiGatewayClient& apiGatewayClient, std::mutex& outputMutex);

    // Blocks reading stdin until either a `login` succeeds (returns a
    // populated LoginOutcome) or the user types "quit"/"exit"/EOF (returns
    // a default-constructed one, loggedIn == false).
    LoginOutcome run();

private:
    WebSocketClient& client;
    ApiGatewayClient& apiGatewayClient;
    std::mutex& outputMutex;

    std::mutex resultMutex;
    std::condition_variable resultCv;
    std::optional<protocol::LoginResult> pendingLoginResult;
    std::optional<protocol::MatchFoundResult> pendingMatchFound;

    void onMessage(const std::string& json);
};

#endif
