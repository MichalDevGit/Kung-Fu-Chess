#ifndef API_GATEWAY_REQUEST_HANDLER_H
#define API_GATEWAY_REQUEST_HANDLER_H

#include <string>

class AuthService;
namespace security
{
    class TokenService;
}

// Translates a raw JSON request body into an AuthService call and a raw
// JSON response body -- the testable business layer behind the REST
// endpoints, same split as server/src/handlers/AuthRequestHandler (parsing/
// dispatch logic here, network transport in ApiGatewayServer). Reuses
// protocol::RegisterRequest/RegisterResult/LoginRequest/LoginResult verbatim
// for both endpoints' bodies -- REST routes by path/method, so these
// structs' own "type" field is simply unused here.
//
// This is the one place in the whole system (MIGRATION_PLAN.md Phase 2)
// that still verifies a password: handleLogin delegates to AuthService,
// then -- only on success -- asks TokenService to issue a signed token the
// WebSocket process (server/) can trust without ever seeing a password
// itself.
class ApiGatewayRequestHandler
{
public:
    ApiGatewayRequestHandler(AuthService& authService, security::TokenService& tokenService);

    // Both never throw -- any parse/validation failure is caught internally
    // and turned into a protocol::ErrorResult envelope instead.
    std::string handleRegister(const std::string& rawJson) const;
    std::string handleLogin(const std::string& rawJson) const;

private:
    AuthService& authService;
    security::TokenService& tokenService;
};

#endif
