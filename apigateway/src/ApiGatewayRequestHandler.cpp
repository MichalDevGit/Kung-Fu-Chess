#include "ApiGatewayRequestHandler.h"

#include <nlohmann/json.hpp>

#include "common/Security/TokenService.h"
#include "common/WallClock.h"
#include "protocol/Message.h"
#include "services/Auth/AuthService.h"

ApiGatewayRequestHandler::ApiGatewayRequestHandler(AuthService& authService, security::TokenService& tokenService)
    : authService(authService)
    , tokenService(tokenService)
{
}

std::string ApiGatewayRequestHandler::handleRegister(const std::string& rawJson) const
{
    try
    {
        const nlohmann::json parsed = nlohmann::json::parse(rawJson);
        const protocol::RegisterRequest request = protocol::RegisterRequest::fromJson(parsed);
        const auth::RegisterOutcome outcome = authService.registerUser(request.username, request.password);
        return protocol::RegisterResult{outcome.success, outcome.userId, outcome.error}.toJson();
    }
    catch (const nlohmann::json::exception&)
    {
        return protocol::ErrorResult{"malformed_request"}.toJson();
    }
}

std::string ApiGatewayRequestHandler::handleLogin(const std::string& rawJson) const
{
    try
    {
        const nlohmann::json parsed = nlohmann::json::parse(rawJson);
        const protocol::LoginRequest request = protocol::LoginRequest::fromJson(parsed);
        const auth::LoginOutcome outcome = authService.login(request.username, request.password);

        if (!outcome.success)
            return protocol::LoginResult{false, 0, 0, outcome.error, ""}.toJson();

        const std::string token = tokenService.issue(outcome.userId, wallClockMillis());
        return protocol::LoginResult{true, outcome.userId, outcome.score, "", token}.toJson();
    }
    catch (const nlohmann::json::exception&)
    {
        return protocol::ErrorResult{"malformed_request"}.toJson();
    }
}
