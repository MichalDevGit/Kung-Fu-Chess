#include "GameSessionManager.h"

#include <utility>

GameSessionManager::GameSessionManager(GameSession::BroadcastFn defaultBroadcast)
    : defaultBroadcast(std::move(defaultBroadcast))
{
}

GameSession& GameSessionManager::getOrCreateDefaultSession()
{
    std::lock_guard<std::mutex> lock(mutex);

    auto it = sessions.find(DEFAULT_SESSION_ID);

    if (it == sessions.end())
    {
        it = sessions.emplace(
            DEFAULT_SESSION_ID,
            std::make_unique<GameSession>(DEFAULT_SESSION_ID, defaultBroadcast))
            .first;
    }

    return *it->second;
}

void GameSessionManager::tickAll(long long milliseconds)
{
    std::lock_guard<std::mutex> lock(mutex);

    for (auto& entry : sessions)
    {
        entry.second->tick(milliseconds);
    }
}
