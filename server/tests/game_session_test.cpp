#include "tests/doctest.h"
#include "services/GameSession.h"
#include "services/GameSessionManager.h"

#include <string>
#include <vector>

TEST_CASE("GameSession broadcasts a GameStartedMessage as soon as it's constructed")
{
    std::vector<std::string> broadcasts;

    GameSession session("test-session", [&broadcasts](const std::string& json)
        {
            broadcasts.push_back(json);
        });

    REQUIRE(broadcasts.size() == 1);
    CHECK(broadcasts[0].find("\"type\":\"game_started\"") != std::string::npos);
}

TEST_CASE("GameSession broadcasts a GameView snapshot on every tick, and again when a move settles, but never from requesting one")
{
    std::vector<std::string> broadcasts;

    GameSession session("test-session", [&broadcasts](const std::string& json)
        {
            broadcasts.push_back(json);
        });

    broadcasts.clear(); // drop the game_started broadcast from construction

    // Classic board: white pawns start on row 6 (see GameFactory::createClassicBoard).
    session.requestMove(Position(6, 4), Position(5, 4));
    CHECK(broadcasts.empty()); // requesting a move doesn't itself fire MoveExecutedEvent

    session.tick(1100); // MILLIS_PER_SQUARE (1000) plus margin -- settles the motion

    // Two broadcasts from this one tick: tick() itself broadcasts
    // unconditionally every time (this is what keeps an in-flight motion's
    // glide smooth on a connected client, since MoveExecutedEvent alone only
    // fires once at the end), plus a second one from MoveExecutedEvent firing
    // because the motion happened to settle during this same tick.
    REQUIRE(broadcasts.size() == 2);
    CHECK(broadcasts[0].find("\"type\":\"game_view\"") != std::string::npos);
    CHECK(broadcasts[1].find("\"type\":\"game_view\"") != std::string::npos);
}

TEST_CASE("GameSession::getGameView/isGameOver reflect this session's own state")
{
    GameSession session("test-session", [](const std::string&) {});

    CHECK(session.isGameOver() == false);
    CHECK(session.getGameView().getBoard().getRows() == 8);
    CHECK(session.getGameView().getBoard().getCols() == 8);
}

TEST_CASE("GameSessionManager returns the same default session across calls")
{
    GameSessionManager manager([](const std::string&) {});

    GameSession& first = manager.getOrCreateDefaultSession();
    GameSession& second = manager.getOrCreateDefaultSession();

    CHECK(&first == &second);
}

TEST_CASE("GameSessionManager::tickAll advances every session's clock")
{
    std::vector<std::string> broadcasts;

    GameSessionManager manager([&broadcasts](const std::string& json)
        {
            broadcasts.push_back(json);
        });

    GameSession& session = manager.getOrCreateDefaultSession();
    broadcasts.clear();

    session.requestMove(Position(6, 4), Position(5, 4));
    manager.tickAll(1100);

    // Same reasoning as the GameSession-level test above: one unconditional
    // per-tick broadcast plus one more from the settling MoveExecutedEvent.
    REQUIRE(broadcasts.size() == 2);
    CHECK(broadcasts[0].find("\"type\":\"game_view\"") != std::string::npos);
    CHECK(broadcasts[1].find("\"type\":\"game_view\"") != std::string::npos);
}
