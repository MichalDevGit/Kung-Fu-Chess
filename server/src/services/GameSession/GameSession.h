#ifndef GAME_SESSION_H
#define GAME_SESSION_H

#include <functional>
#include <mutex>
#include <optional>
#include <string>

#include "../../../../common/EventBus/EventBus.h"
#include "../../../../common/DTO/GameView.h"
#include "../../../../common/enums/PieceColor.h"
#include "../../game/Engine/GameEngine.h"
#include "../../game/Controller/Controller.h"
#include "../../game/model/Position.h"

// Owns one authoritative 1v1 match: its own EventBus, GameEngine, and
// Controller, plus exactly two participants (assigned White/Black at
// construction by GameSessionManager::createSession, always in matchmaking
// queue order -- see Matchmaker -- so White really is "whoever entered
// first"). This is the seam where "UI and logic talk in-process" (the old
// src/main.cpp wiring, before the client/server split) becomes "network
// client and authoritative server talk over JSON."
//
// A participant's connection id is mutable (see markReconnected) -- a
// dropped connection doesn't end the game, it just stops delivery to that
// slot until either a reconnect rebinds it or tick() decides the grace
// period (MatchmakingConfig::RECONNECT_GRACE_MILLIS) has run out and
// forfeits the game to whoever's still connected.
//
// Guards every public method with its own mutex: once wired behind
// WebSocketServer, requestMove/requestJump/tick/markDisconnected/
// markReconnected can all be called concurrently (one client's request
// thread, the server's own tick-loop thread) against the same GameEngine,
// which has no locking of its own -- same plain-mutex pattern AuthService
// already uses around its IUserRepository calls.
class GameSession
{
public:
    using SendToFn = std::function<void(const std::string& connectionId, const std::string& json)>;

    // Applies a game's outcome (winner/loser user ids) as an ELO rating
    // update on game-over -- injected rather than holding a RatingService&
    // directly, so GameSession itself never needs to know persistence exists
    // (same discipline SendToFn already keeps it from knowing
    // WebSocketServer/ixwebsocket exist). Both game-ending paths (an ordinary
    // king-capture win and a disconnect-timeout forfeit) call this same
    // callback -- see forfeitTo() and the GameOverEvent subscriber.
    using GameOutcomeFn = std::function<void(int winnerUserId, int loserUserId)>;

    struct Player
    {
        int userId = 0;
        std::string username;
        std::string connectionId;
    };

    // Outcome of a requestMove/requestJump call -- distinct from
    // MoveValidation (server/src/game/model/../common/DTO/MoveValidation.h),
    // which is about geometry/rules; this is about *who's allowed to ask at
    // all*, checked before the engine is ever touched.
    struct CommandOutcome
    {
        bool accepted = false;
        std::string reason; // e.g. "not_your_piece", "unknown_connection" -- empty when accepted
    };

    GameSession(std::string id, Player white, Player black, SendToFn sendTo, GameOutcomeFn gameOutcome);

    const std::string& getId() const;

    CommandOutcome requestMove(const std::string& connectionId, const Position& from, const Position& to);
    CommandOutcome requestJump(const std::string& connectionId, const Position& position);

    // Advances this session's authoritative clock. Called by the server's
    // own tick loop (see GameSessionManager::tickAll), never by a client
    // request -- real-time cooldowns must progress whether or not any
    // client happens to be connected right now. Also checks whether a
    // disconnected participant's grace period has expired and, if so,
    // forfeits the game (applies the ELO rating update via gameOutcome,
    // pushes GameOverMessage, marks this session finished).
    void tick(long long milliseconds);

    // connectionId no longer belongs to whichever participant it was bound
    // to (that slot's messages are simply not delivered until a reconnect).
    // No-op if connectionId isn't a current participant.
    void markDisconnected(const std::string& connectionId);

    // Rebinds whichever slot belongs to userId to newConnectionId and
    // clears that slot's disconnected timer. No-op if userId isn't a
    // participant of this session.
    void markReconnected(int userId, const std::string& newConnectionId);

    bool hasConnection(const std::string& connectionId) const;
    bool hasUser(int userId) const;

    struct ResumeInfo
    {
        PieceColor color = PieceColor::None;
        std::string opponentUsername;
    };

    // Nullopt if userId isn't a participant. Used by AuthRequestHandler to
    // rebuild a MatchFoundResult-shaped push for a reconnecting player,
    // without needing to know GameSession's internals.
    std::optional<ResumeInfo> resumeInfoFor(int userId) const;

    GameView getGameView() const;
    bool isGameOver() const;

    // True once this session has ended for any reason -- a king capture
    // (isGameOver() is also true in that case) or a disconnect-timeout
    // forfeit (see tick()/forfeitTo(), where isGameOver() stays false since
    // no king was actually captured). GameSessionManager uses this as the
    // one signal for "safe to drop from the session map," regardless of
    // which of the two endings actually happened.
    bool isFinished() const;

private:
    std::string id;
    Player white;
    Player black;
    long long whiteDisconnectedAtMs = 0; // 0 == currently connected
    long long blackDisconnectedAtMs = 0;
    bool finished = false;

    SendToFn sendTo;
    GameOutcomeFn gameOutcome;

    mutable std::mutex mutex;
    EventBus eventBus;
    GameEngine engine;
    Controller controller;

    void subscribeToEvents();
    void sendToParticipants(const std::string& json);

    // Returns nullptr if connectionId belongs to neither participant.
    Player* playerByConnection(const std::string& connectionId);
    PieceColor colorOf(const Player* player) const; // player == &white ? White : Black

    // Runs with `mutex` already held -- ends the game in favor of `winner`,
    // applies the ELO rating update via gameOutcome, and pushes
    // GameOverMessage. Shared by both the forfeit-timeout path (tick()) and,
    // later, any other reason a session might need to end without a king
    // capture.
    void forfeitTo(const Player& winner, const Player& loser);
};

#endif
