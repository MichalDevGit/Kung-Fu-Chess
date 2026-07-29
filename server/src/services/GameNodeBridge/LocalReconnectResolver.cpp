#include "LocalReconnectResolver.h"

#include "services/GameSession/GameSession.h"
#include "services/GameSession/GameSessionManager.h"

LocalReconnectResolver::LocalReconnectResolver(GameSessionManager& sessionManager) : sessionManager(sessionManager)
{
}

std::optional<protocol::MatchFoundResult> LocalReconnectResolver::checkAndRebind(
    int userId, const std::string& newConnectionId)
{
    GameSession* existingSession = sessionManager.findSessionByUserId(userId);
    if (existingSession == nullptr)
        return std::nullopt;

    sessionManager.rebindConnection(userId, newConnectionId);

    const std::optional<GameSession::ResumeInfo> resumeInfo = existingSession->resumeInfoFor(userId);
    if (!resumeInfo.has_value())
        return std::nullopt;

    return protocol::MatchFoundResult{
        existingSession->getId(), resumeInfo->color, resumeInfo->opponentUsername, existingSession->getGameView()};
}
