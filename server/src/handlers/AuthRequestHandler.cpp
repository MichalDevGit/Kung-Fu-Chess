#include "handlers/AuthRequestHandler.h"

#include <nlohmann/json.hpp>
#include <optional>
#include <utility>

#include "protocol/Message.h"
#include "protocol/MessageType.h"
#include "common/Security/TokenService.h"
#include "common/WallClock.h"
#include "persistence/IUserRepository.h"
#include "persistence/UserRecord.h"
#include "services/ConnectionRegistry.h"
#include "services/GameSession.h"
#include "services/GameSessionManager.h"

AuthRequestHandler::AuthRequestHandler(
    IUserRepository& users,
    security::TokenService& tokenService,
    ConnectionRegistry& connectionRegistry,
    GameSessionManager& sessionManager,
    SendFn sendResume)
    : users(users)
    , tokenService(tokenService)
    , connectionRegistry(connectionRegistry)
    , sessionManager(sessionManager)
    , sendResume(std::move(sendResume))
{
}

std::string AuthRequestHandler::completeLogin(
    const std::string& connectionId, int userId, const std::string& username, int score) const
{
    connectionRegistry.onAuthenticated(connectionId, ConnectionRegistry::AuthenticatedUser{userId, username, score});

    // A returning player (their connection dropped mid-game and they're
    // logging back in) resumes their existing session instead of being sent
    // through matchmaking again -- see the class comment for why this makes
    // find_game-right-after-login always safe on the client side.
    GameSession* existingSession = sessionManager.findSessionByUserId(userId);
    if (existingSession != nullptr)
    {
        sessionManager.rebindConnection(userId, connectionId);

        const std::optional<GameSession::ResumeInfo> resumeInfo = existingSession->resumeInfoFor(userId);
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

    // token is always empty here -- only the API Gateway's REST /login
    // issues tokens (see MIGRATION_PLAN.md Phase 2); this process only ever
    // verifies one.
    return protocol::LoginResult{true, userId, score, "", ""}.toJson();
}

std::string AuthRequestHandler::handle(const std::string& connectionId, const std::string& rawJson) const
{
    try
    {
        const nlohmann::json parsed = nlohmann::json::parse(rawJson);
        const std::string type = protocol::readType(parsed);

        if (type == protocol::MessageType::LoginWithToken)
        {
            const protocol::LoginWithTokenRequest request = protocol::LoginWithTokenRequest::fromJson(parsed);
            const security::VerifyResult verified = tokenService.verify(request.token, wallClockMillis());

            if (!verified.valid)
                return protocol::LoginResult{false, 0, 0, verified.error, ""}.toJson();

            const std::optional<UserRecord> user = users.findById(verified.userId);
            if (!user.has_value())
                return protocol::LoginResult{false, 0, 0, "unknown_user", ""}.toJson();

            return completeLogin(connectionId, user->id, user->username, user->score);
        }

        return protocol::ErrorResult{"unknown_type"}.toJson();
    }
    catch (const nlohmann::json::exception&)
    {
        return protocol::ErrorResult{"malformed_request"}.toJson();
    }
}
