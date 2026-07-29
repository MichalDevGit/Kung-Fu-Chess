#include "GameSessionManager.h"

#include <utility>

#include "../../persistence/IUserRepository.h"

GameSessionManager::GameSessionManager(GameSession::SendToFn sendTo, IUserRepository& userRepository, ISessionIndexStore& indexStore)
    : sendTo(std::move(sendTo))
    , ratingService(userRepository)
    , indexStore(indexStore)
{
}

GameSession& GameSessionManager::createSession(GameSession::Player white, GameSession::Player black)
{
    std::lock_guard<std::mutex> lock(mutex);

    const std::string sessionId = "session-" + std::to_string(nextSessionNumber++);

    RatingService& rating = ratingService;
    auto gameOutcome = [&rating](int winnerUserId, int loserUserId)
    {
        rating.applyGameResult(winnerUserId, loserUserId);
    };

    const std::string whiteConnectionId = white.connectionId;
    const std::string blackConnectionId = black.connectionId;
    const int whiteUserId = white.userId;
    const int blackUserId = black.userId;

    auto session = std::make_unique<GameSession>(sessionId, std::move(white), std::move(black), sendTo, gameOutcome);
    GameSession& ref = *session;
    sessions.emplace(sessionId, std::move(session));

    indexStore.bindSession(sessionId, {whiteConnectionId, blackConnectionId}, {whiteUserId, blackUserId});

    return ref;
}

GameSession* GameSessionManager::findSessionByConnection(const std::string& connectionId)
{
    std::lock_guard<std::mutex> lock(mutex);

    const std::optional<std::string> sessionId = indexStore.findSessionIdByConnection(connectionId);
    if (!sessionId.has_value())
        return nullptr;

    const auto sessionIt = sessions.find(*sessionId);
    return sessionIt != sessions.end() ? sessionIt->second.get() : nullptr;
}

GameSession* GameSessionManager::findSessionByUserId(int userId)
{
    std::lock_guard<std::mutex> lock(mutex);

    const std::optional<std::string> sessionId = indexStore.findSessionIdByUserId(userId);
    if (!sessionId.has_value())
        return nullptr;

    const auto sessionIt = sessions.find(*sessionId);
    return sessionIt != sessions.end() ? sessionIt->second.get() : nullptr;
}

bool GameSessionManager::rebindConnection(int userId, const std::string& newConnectionId)
{
    std::lock_guard<std::mutex> lock(mutex);

    const std::optional<std::string> sessionId = indexStore.findSessionIdByUserId(userId);
    if (!sessionId.has_value())
        return false;

    const auto sessionIt = sessions.find(*sessionId);
    if (sessionIt == sessions.end())
        return false;

    sessionIt->second->markReconnected(userId, newConnectionId);
    indexStore.bindConnection(newConnectionId, *sessionId);
    return true;
}

void GameSessionManager::onConnectionClosed(const std::string& connectionId)
{
    std::lock_guard<std::mutex> lock(mutex);

    const std::optional<std::string> sessionId = indexStore.findSessionIdByConnection(connectionId);
    if (!sessionId.has_value())
        return;

    const auto sessionIt = sessions.find(*sessionId);
    if (sessionIt != sessions.end())
        sessionIt->second->markDisconnected(connectionId);

    // This exact connection is dead either way -- drop the route to it
    // regardless of whether the session lookup above succeeded, so a stale
    // entry can never linger past this call.
    indexStore.unbindConnection(connectionId);
}

void GameSessionManager::tickAll(long long milliseconds)
{
    std::lock_guard<std::mutex> lock(mutex);

    for (auto& entry : sessions)
        entry.second->tick(milliseconds);

    removeFinishedSessions();
}

void GameSessionManager::removeFinishedSessions()
{
    // Called with `mutex` already held.
    for (auto it = sessions.begin(); it != sessions.end();)
    {
        if (!it->second->isFinished())
        {
            ++it;
            continue;
        }

        indexStore.unbindSession(it->first);
        it = sessions.erase(it);
    }
}
