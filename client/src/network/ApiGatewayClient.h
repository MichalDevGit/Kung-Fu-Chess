#ifndef API_GATEWAY_CLIENT_H
#define API_GATEWAY_CLIENT_H

#include <string>

#include "protocol/Message.h"

// The one file that talks to the API Gateway's REST endpoints (see
// MIGRATION_PLAN.md Phase 2) -- register/login now happen here, over plain
// HTTP, instead of as WebSocket messages. Mirrors WebSocketClient's framing
// as "the only file that includes this specific ixwebsocket API" (here,
// ix::HttpClient rather than ix::WebSocket). Both calls are synchronous --
// simpler than the WS round-trip they replace, since HTTP request/response
// is naturally request-then-reply, with no need for CliShell's
// condition-variable/timeout dance the old WS-native login required.
class ApiGatewayClient
{
public:
    // baseUrl looks like "http://127.0.0.1:9004" (no trailing slash).
    explicit ApiGatewayClient(std::string baseUrl);

    protocol::RegisterResult registerUser(const std::string& username, const std::string& password) const;

    // On success, the returned LoginResult.token is what the caller should
    // send over the WebSocket connection as a LoginWithTokenRequest -- this
    // class itself makes no WebSocket calls at all.
    protocol::LoginResult login(const std::string& username, const std::string& password) const;

private:
    std::string baseUrl;
};

#endif
