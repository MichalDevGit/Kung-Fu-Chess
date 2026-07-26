#ifndef GAME_SESSION_H
#define GAME_SESSION_H

#include <functional>
#include <mutex>
#include <string>

#include "../../../common/EventBus/EventBus.h"
#include "../../../common/DTO/GameView.h"
#include "../game/Engine/GameEngine.h"
#include "../game/Controller/Controller.h"
#include "../game/model/Position.h"

// Owns one authoritative game: its own EventBus, GameEngine, and Controller.
// This is the seam where "UI and logic talk in-process" (the old
// src/main.cpp wiring, before the client/server split) becomes "network
// client and authoritative server talk over JSON" -- it subscribes to the
// EventBus events GameEngine already fires (see common/EventBus/Events.h)
// and serializes each into the matching protocol message via an injected
// broadcast callback, so however those messages actually reach connected
// clients (today: nothing real yet: see GameSessionManager; later: a real
// per-connection push) is a detail this class doesn't need to know.
//
// Guards every public method with its own mutex: once wired behind
// WebSocketServer, requestMove/requestJump/tick can all be called
// concurrently (one client's request thread, the server's own tick-loop
// thread) against the same GameEngine, which has no locking of its own --
// same plain-mutex pattern AuthService already uses for UserRepository's
// shared connection.
class GameSession
{
public:
    using BroadcastFn = std::function<void(const std::string& json)>;

    GameSession(std::string id, BroadcastFn broadcast);

    const std::string& getId() const;

    void requestMove(const Position& from, const Position& to);
    void requestJump(const Position& position);

    // Advances this session's authoritative clock. Called by the server's
    // own tick loop (see GameSessionManager::tickAll), never by a client
    // request -- real-time cooldowns must progress whether or not any
    // client happens to be connected right now.
    void tick(long long milliseconds);

    GameView getGameView() const;
    bool isGameOver() const;

private:
    std::string id;
    BroadcastFn broadcast;
    mutable std::mutex mutex;
    EventBus eventBus;
    GameEngine engine;
    Controller controller;

    void subscribeToEvents();
};

#endif
