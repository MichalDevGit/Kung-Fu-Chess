#include "GameSessionManager.h"

#include <utility>

#include "../persistence/IUserRepository.h"

GameSessionManager::GameSessionManager(GameSession::SendToFn sendTo, IUserRepository& userRepository)
    : sendTo(std::move(sendTo))
    , ratingService(userRepository)
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

    sessionIdByConnection[whiteConnectionId] = sessionId;
    sessionIdByConnection[blackConnectionId] = sessionId;
    sessionIdByUserId[whiteUserId] = sessionId;
    sessionIdByUserId[blackUserId] = sessionId;

    return ref;
}

GameSession* GameSessionManager::findSessionByConnection(const std::string& connectionId)
{
    std::lock_guard<std::mutex> lock(mutex);

    const auto it = sessionIdByConnection.find(connectionId);
    if (it == sessionIdByConnection.end())
        return nullptr;

    const auto sessionIt = sessions.find(it->second);
    return sessionIt != sessions.end() ? sessionIt->second.get() : nullptr;
}

GameSession* GameSessionManager::findSessionByUserId(int userId)
{
    std::lock_guard<std::mutex> lock(mutex);

    const auto it = sessionIdByUserId.find(userId);
    if (it == sessionIdByUserId.end())
        return nullptr;

    const auto sessionIt = sessions.find(it->second);
    return sessionIt != sessions.end() ? sessionIt->second.get() : nullptr;
}

bool GameSessionManager::rebindConnection(int userId, const std::string& newConnectionId)
{
    std::lock_guard<std::mutex> lock(mutex);

    const auto it = sessionIdByUserId.find(userId);
    if (it == sessionIdByUserId.end())
        return false;

    const auto sessionIt = sessions.find(it->second);
    if (sessionIt == sessions.end())
        return false;

    sessionIt->second->markReconnected(userId, newConnectionId);
    sessionIdByConnection[newConnectionId] = it->second;
    return true;
}

void GameSessionManager::onConnectionClosed(const std::string& connectionId)
{
    std::lock_guard<std::mutex> lock(mutex);

    const auto it = sessionIdByConnection.find(connectionId);
    if (it == sessionIdByConnection.end())
        return;

    const auto sessionIt = sessions.find(it->second);
    if (sessionIt != sessions.end())
        sessionIt->second->markDisconnected(connectionId);

    // This exact connection is dead either way -- drop the route to it
    // regardless of whether the session lookup above succeeded, so a stale
    // entry can never linger past this call.
    sessionIdByConnection.erase(it);
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

        const std::string& finishedId = it->first;

        for (auto connIt = sessionIdByConnection.begin(); connIt != sessionIdByConnection.end();)
            connIt = (connIt->second == finishedId) ? sessionIdByConnection.erase(connIt) : std::next(connIt);

        for (auto userIt = sessionIdByUserId.begin(); userIt != sessionIdByUserId.end();)
            userIt = (userIt->second == finishedId) ? sessionIdByUserId.erase(userIt) : std::next(userIt);

        it = sessions.erase(it);
    }
}
