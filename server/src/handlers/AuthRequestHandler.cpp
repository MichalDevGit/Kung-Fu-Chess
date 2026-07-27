#include "handlers/AuthRequestHandler.h"

#include <nlohmann/json.hpp>
#include <optional>
#include <utility>

#include "protocol/Message.h"
#include "protocol/MessageType.h"
#include "services/AuthService.h"
#include "services/ConnectionRegistry.h"
#include "services/GameSession.h"
#include "services/GameSessionManager.h"

AuthRequestHandler::AuthRequestHandler(
    AuthService& authService, ConnectionRegistry& connectionRegistry, GameSessionManager& sessionManager, SendFn sendResume)
    : authService(authService)
    , connectionRegistry(connectionRegistry)
    , sessionManager(sessionManager)
    , sendResume(std::move(sendResume))
{
}

std::string AuthRequestHandler::handle(const std::string& connectionId, const std::string& rawJson) const
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

            if (outcome.success)
            {
                connectionRegistry.onAuthenticated(
                    connectionId,
                    ConnectionRegistry::AuthenticatedUser{outcome.userId, request.username, outcome.score});

                // A returning player (their connection dropped mid-game and
                // they're logging back in) resumes their existing session
                // instead of being sent through matchmaking again -- see the
                // class comment for why this makes find_game-right-after-
                // login always safe on the client side.
                GameSession* existingSession = sessionManager.findSessionByUserId(outcome.userId);
                if (existingSession != nullptr)
                {
                    sessionManager.rebindConnection(outcome.userId, connectionId);

                    const std::optional<GameSession::ResumeInfo> resumeInfo = existingSession->resumeInfoFor(outcome.userId);
                    if (resumeInfo.has_value())
                    {
                        sendResume(
                            connectionId,
                            protocol::MatchFoundResult{
                                existingSession->getId(),
                                resumeInfo->color,
                                resumeInfo->opponentUsername,
                                existingSession->getGameView()}
                                .toJson());
                    }
                }
            }

            return protocol::LoginResult{outcome.success, outcome.userId, outcome.score, outcome.error}.toJson();
        }

        return protocol::ErrorResult{"unknown_type"}.toJson();
    }
    catch (const nlohmann::json::exception&)
    {
        return protocol::ErrorResult{"malformed_request"}.toJson();
    }
}
