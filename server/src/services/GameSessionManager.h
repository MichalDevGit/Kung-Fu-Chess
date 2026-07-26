#ifndef GAME_SESSION_MANAGER_H
#define GAME_SESSION_MANAGER_H

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "GameSession.h"

// Owns every active GameSession, keyed by session id. This is the extension
// point for going from "one game" to "many concurrent games" later: today it
// only ever exposes one implicit session (see getOrCreateDefaultSession),
// but it's already a real id -> session map, so adding real join/create
// requests that pick a specific session by id is additive to this class, not
// a redesign of it.
//
// Guards the session map itself with its own mutex (separate from each
// GameSession's own mutex, which guards that session's engine/controller):
// the server's tick-loop thread iterates this map on every tick while
// request-handling threads can concurrently insert into it via
// getOrCreateDefaultSession -- unsynchronized, that's a data race on the map
// itself, not just on a session's game state.
class GameSessionManager
{
public:
    explicit GameSessionManager(GameSession::BroadcastFn defaultBroadcast);

    // Returns the single implicit session, creating it (with the broadcast
    // callback supplied at construction) on first call.
    GameSession& getOrCreateDefaultSession();

    // Advances every active session's clock by the same interval -- the
    // server-owned replacement for the old GameLoop-driven wall-clock
    // controller.wait(deltaMs) call (see TimingConfig::SERVER_TICK_INTERVAL_MILLIS).
    void tickAll(long long milliseconds);

private:
    static constexpr const char* DEFAULT_SESSION_ID = "default";

    std::mutex mutex;
    GameSession::BroadcastFn defaultBroadcast;
    std::unordered_map<std::string, std::unique_ptr<GameSession>> sessions;
};

#endif
