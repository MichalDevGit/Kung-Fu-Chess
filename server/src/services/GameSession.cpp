#include "GameSession.h"

#include <utility>

#include "../game/IO/GameFactory.h"
#include "../../../common/EventBus/Events.h"
#include "protocol/Message.h"

GameSession::GameSession(std::string id, BroadcastFn broadcast)
    : id(std::move(id)),
      broadcast(std::move(broadcast)),
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

void GameSession::requestMove(const Position& from, const Position& to)
{
    std::lock_guard<std::mutex> lock(mutex);
    controller.move(from, to);
}

void GameSession::requestJump(const Position& position)
{
    std::lock_guard<std::mutex> lock(mutex);
    controller.jump(position);
}

void GameSession::tick(long long milliseconds)
{
    std::lock_guard<std::mutex> lock(mutex);
    controller.wait(milliseconds);

    // Broadcast unconditionally, not just on MoveExecutedEvent/PieceCapturedEvent
    // (which only fire once a motion/jump *finishes*) -- an in-flight motion
    // needs a fresh view every tick for its client-side glide to look
    // continuous instead of teleporting from start to end.
    broadcast(protocol::GameViewMessage{controller.getGameView()}.toJson());
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
            broadcast(protocol::GameStartedMessage{}.toJson());
        });

    eventBus.subscribe<GameOverEvent>([this](const GameOverEvent&)
        {
            broadcast(protocol::GameOverMessage{}.toJson());
        });

    // MoveExecutedEvent/PieceCapturedEvent have no dedicated wire message --
    // a fresh GameView snapshot already conveys the resulting state fully,
    // so both just trigger the same broadcast rather than inventing a
    // redundant message shape per event.
    eventBus.subscribe<MoveExecutedEvent>([this](const MoveExecutedEvent&)
        {
            broadcast(protocol::GameViewMessage{controller.getGameView()}.toJson());
        });

    eventBus.subscribe<PieceCapturedEvent>([this](const PieceCapturedEvent&)
        {
            broadcast(protocol::GameViewMessage{controller.getGameView()}.toJson());
        });
}
