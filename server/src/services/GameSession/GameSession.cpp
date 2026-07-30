#include "GameSession.h"

#include <utility>

#include "../../game/IO/GameFactory.h"
#include "../../../../common/EventBus/Events.h"
#include "../../../../common/MonotonicClock.h"
#include "../../../../common/Config/MatchmakingConfig.h"
#include "protocol/Message.h"

GameSession::GameSession(std::string id, Player white, Player black, SendToFn sendTo, GameOutcomeFn gameOutcome)
    : id(std::move(id)),
      white(std::move(white)),
      black(std::move(black)),
      sendTo(std::move(sendTo)),
      gameOutcome(std::move(gameOutcome)),
      eventBus(),
      engine(GameFactory::createNewGame(eventBus)),
      controller(engine)
{
    subscribeToEvents();

    // Matches the old src/main.cpp wiring this class replaces: GameEngine/
    // GameFactory never publish this themselves (see GameEngine.cpp), it's
    // the constructing owner's job to signal "a new game just started" once
    // everything is wired up.
    eventBus.publish(GameStartedEvent{});
}

const std::string& GameSession::getId() const
{
    return id;
}

std::vector<int> GameSession::userIds() const
{
    return {white.userId, black.userId};
}

GameSession::Player* GameSession::playerByConnection(const std::string& connectionId)
{
    if (white.connectionId == connectionId)
        return &white;
    if (black.connectionId == connectionId)
        return &black;
    return nullptr;
}

PieceColor GameSession::colorOf(const Player* player) const
{
    return player == &white ? PieceColor::White : PieceColor::Black;
}

GameSession::CommandOutcome GameSession::requestMove(const std::string& connectionId, const Position& from, const Position& to)
{
    std::lock_guard<std::mutex> lock(mutex);

    Player* player = playerByConnection(connectionId);
    if (player == nullptr)
        return CommandOutcome{false, "unknown_connection"};

    if (controller.pieceColorAt(from) != colorOf(player))
        return CommandOutcome{false, "not_your_piece"};

    controller.move(from, to);
    return CommandOutcome{true, {}};
}

GameSession::CommandOutcome GameSession::requestJump(const std::string& connectionId, const Position& position)
{
    std::lock_guard<std::mutex> lock(mutex);

    Player* player = playerByConnection(connectionId);
    if (player == nullptr)
        return CommandOutcome{false, "unknown_connection"};

    if (controller.pieceColorAt(position) != colorOf(player))
        return CommandOutcome{false, "not_your_piece"};

    controller.jump(position);
    return CommandOutcome{true, {}};
}

void GameSession::tick(long long milliseconds)
{
    std::lock_guard<std::mutex> lock(mutex);

    if (finished)
        return;

    controller.wait(milliseconds);

    // Broadcast unconditionally, not just on MoveExecutedEvent/PieceCapturedEvent
    // (which only fire once a motion/jump *finishes*) -- an in-flight motion
    // needs a fresh view every tick for its client-side glide to look
    // continuous instead of teleporting from start to end.
    sendToParticipants(protocol::GameViewMessage{controller.getGameView()}.toJson());

    const long long now = nowMillis();

    if (whiteDisconnectedAtMs != 0 && now - whiteDisconnectedAtMs >= MatchmakingConfig::RECONNECT_GRACE_MILLIS)
        forfeitTo(black, white);
    else if (blackDisconnectedAtMs != 0 && now - blackDisconnectedAtMs >= MatchmakingConfig::RECONNECT_GRACE_MILLIS)
        forfeitTo(white, black);
}

void GameSession::markDisconnected(const std::string& connectionId)
{
    std::lock_guard<std::mutex> lock(mutex);

    if (finished)
        return;

    Player* player = playerByConnection(connectionId);
    if (player == nullptr)
        return;

    const long long now = nowMillis();
    if (player == &white)
        whiteDisconnectedAtMs = now;
    else
        blackDisconnectedAtMs = now;

    sendToParticipants(protocol::OpponentDisconnectedMessage{}.toJson());
}

void GameSession::markReconnected(int userId, const std::string& newConnectionId)
{
    std::lock_guard<std::mutex> lock(mutex);

    if (finished)
        return;

    Player* player = nullptr;
    if (white.userId == userId)
        player = &white;
    else if (black.userId == userId)
        player = &black;

    if (player == nullptr)
        return;

    player->connectionId = newConnectionId;
    if (player == &white)
        whiteDisconnectedAtMs = 0;
    else
        blackDisconnectedAtMs = 0;

    // Only the *other* participant should hear "opponent reconnected" --
    // sendToParticipants would also deliver it to the reconnecting player
    // themselves (their slot now shows as connected again), which reads as
    // "you reconnected" nonsensically restated back at them.
    const bool reconnectedWasWhite = (player == &white);
    const Player& other = reconnectedWasWhite ? black : white;
    const long long otherDisconnectedAtMs = reconnectedWasWhite ? blackDisconnectedAtMs : whiteDisconnectedAtMs;

    if (otherDisconnectedAtMs == 0)
        sendTo(other.connectionId, protocol::OpponentReconnectedMessage{}.toJson());
}

bool GameSession::hasConnection(const std::string& connectionId) const
{
    std::lock_guard<std::mutex> lock(mutex);
    return connectionId == white.connectionId || connectionId == black.connectionId;
}

bool GameSession::hasUser(int userId) const
{
    std::lock_guard<std::mutex> lock(mutex);
    return userId == white.userId || userId == black.userId;
}

std::optional<GameSession::ResumeInfo> GameSession::resumeInfoFor(int userId) const
{
    std::lock_guard<std::mutex> lock(mutex);

    if (userId == white.userId)
        return ResumeInfo{PieceColor::White, black.username};
    if (userId == black.userId)
        return ResumeInfo{PieceColor::Black, white.username};
    return std::nullopt;
}

GameView GameSession::getGameView() const
{
    std::lock_guard<std::mutex> lock(mutex);
    return controller.getGameView();
}

bool GameSession::isGameOver() const
{
    std::lock_guard<std::mutex> lock(mutex);
    return controller.isGameOver();
}

bool GameSession::isFinished() const
{
    std::lock_guard<std::mutex> lock(mutex);
    return finished;
}

void GameSession::forfeitTo(const Player& winner, const Player& loser)
{
    // Called with `mutex` already held (from tick()).
    finished = true;

    gameOutcome(winner.userId, loser.userId);

    sendToParticipants(protocol::GameOverMessage{"opponent_disconnected", winner.userId}.toJson());
}

void GameSession::subscribeToEvents()
{
    // These handlers only ever run synchronously from inside a call already
    // holding `mutex` (EventBus::publish() is called directly from
    // GameEngine, which only this session's own already-locked
    // requestMove/requestJump/tick ever touch) -- so they read `controller`
    // directly rather than through this class's own locking public methods,
    // to avoid re-locking a non-recursive mutex.

    eventBus.subscribe<GameStartedEvent>([this](const GameStartedEvent&)
        {
            sendToParticipants(protocol::GameStartedMessage{}.toJson());
        });

    eventBus.subscribe<GameOverEvent>([this](const GameOverEvent& event)
        {
            finished = true;

            const bool whiteLost = event.loserColor == PieceColor::White;
            const Player& loser = whiteLost ? white : black;
            const Player& winner = whiteLost ? black : white;
            gameOutcome(winner.userId, loser.userId);

            sendToParticipants(protocol::GameOverMessage{"", winner.userId}.toJson());
        });

    // MoveExecutedEvent/PieceCapturedEvent have no dedicated wire message --
    // a fresh GameView snapshot already conveys the resulting state fully,
    // so both just trigger the same broadcast rather than inventing a
    // redundant message shape per event.
    eventBus.subscribe<MoveExecutedEvent>([this](const MoveExecutedEvent&)
        {
            sendToParticipants(protocol::GameViewMessage{controller.getGameView()}.toJson());
        });

    eventBus.subscribe<PieceCapturedEvent>([this](const PieceCapturedEvent&)
        {
            sendToParticipants(protocol::GameViewMessage{controller.getGameView()}.toJson());
        });
}

void GameSession::sendToParticipants(const std::string& json)
{
    if (whiteDisconnectedAtMs == 0)
        sendTo(white.connectionId, json);
    if (blackDisconnectedAtMs == 0)
        sendTo(black.connectionId, json);
}
