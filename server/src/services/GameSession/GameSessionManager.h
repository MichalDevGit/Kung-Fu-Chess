#ifndef GAME_SESSION_MANAGER_H
#define GAME_SESSION_MANAGER_H

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "GameSession.h"
#include "../SessionIndex/ISessionIndexStore.h"
#include "../Rating/RatingService.h"

class IUserRepository;

// Owns every active GameSession, keyed by session id, plus the reverse
// indexes (connectionId -> sessionId, userId -> sessionId) that let a
// move/jump request or a reconnecting login find "which session is this"
// in O(1) instead of scanning every session. Real multi-game support (this
// class's whole reason to exist over a single implicit session): each
// GameSession is created once, by Matchmaker pairing two queued
// connections (see server/main.cpp), never lazily by the first request that
// happens to arrive.
//
// Guards the session map/indexes themselves with its own mutex (separate
// from each GameSession's own mutex, which guards that session's engine/
// controller): the server's tick-loop thread iterates this map on every
// tick while request-handling threads can concurrently insert into it via
// createSession -- unsynchronized, that's a data race on the map itself,
// not just on a session's game state.
class GameSessionManager
{
public:
    // sendTo is shared by every session this manager creates (each
    // GameSession only ever calls it with its own two participants'
    // connection ids). userRepository backs the RatingService every session's
    // GameOutcomeFn closes over, for ELO rating updates on either
    // game-ending path (see GameSession::forfeitTo and its GameOverEvent
    // subscriber). indexStore backs the connectionId/userId -> sessionId
    // lookup indexes (LocalSessionIndexStore by default, RedisSessionIndexStore
    // for a multi-node deployment -- see server/src/main.cpp's
    // KUNGFUCHESS_REDIS_URL wiring); the `sessions` map of live GameSession
    // objects itself stays in-process only, unaffected by this.
    GameSessionManager(GameSession::SendToFn sendTo, IUserRepository& userRepository, ISessionIndexStore& indexStore);

    GameSession& createSession(GameSession::Player white, GameSession::Player black);

    GameSession* findSessionByConnection(const std::string& connectionId);
    GameSession* findSessionByUserId(int userId);

    // Rebinds userId's session (if any) to newConnectionId -- used when a
    // fresh login recognizes a returning player instead of sending them
    // through matchmaking again. No-op (returns false) if userId has no
    // active session.
    bool rebindConnection(int userId, const std::string& newConnectionId);

    // Marks whichever session connectionId belongs to (if any) as having
    // that participant disconnected. No-op if connectionId isn't part of
    // any active session.
    void onConnectionClosed(const std::string& connectionId);

    // Advances every active session's clock by the same interval -- the
    // server-owned replacement for the old GameLoop-driven wall-clock
    // controller.wait(deltaMs) call (see TimingConfig::SERVER_TICK_INTERVAL_MILLIS).
    // Also drops any session that finished (game-over or forfeit) during
    // this tick from the map/indexes.
    void tickAll(long long milliseconds);

private:
    std::mutex mutex;
    GameSession::SendToFn sendTo;
    RatingService ratingService;
    ISessionIndexStore& indexStore;
    long long nextSessionNumber = 1;

    std::unordered_map<std::string, std::unique_ptr<GameSession>> sessions;

    void removeFinishedSessions();
};

#endif
