#include "handlers/AuthRequestHandler.h"

#include <nlohmann/json.hpp>

#include "protocol/Message.h"
#include "protocol/MessageType.h"
#include "services/AuthService.h"

AuthRequestHandler::AuthRequestHandler(AuthService& authService)
    : authService(authService)
{
}

std::string AuthRequestHandler::handle(const std::string& rawJson) const
{
    try
    {
        const nlohmann::json parsed = nlohmann::json::parse(rawJson);
        const std::string type = protocol::readType(parsed);

        if (type == protocol::MessageType::Register)
        {
            const protocol::RegisterRequest request = protocol::RegisterRequest::fromJson(parsed);
            const auth::RegisterOutcome outcome = authService.registerUser(request.username, request.password);
            return protocol::RegisterResult{outcome.success, outcome.userId, outcome.error}.toJson();
        }

        if (type == protocol::MessageType::Login)
        {
            const protocol::LoginRequest request = protocol::LoginRequest::fromJson(parsed);
            const auth::LoginOutcome outcome = authService.login(request.username, request.password);
            return protocol::LoginResult{outcome.success, outcome.userId, outcome.score, outcome.error}.toJson();
        }

        return protocol::ErrorResult{"unknown_type"}.toJson();
    }
    catch (const nlohmann::json::exception&)
    {
        return protocol::ErrorResult{"malformed_request"}.toJson();
    }
}
