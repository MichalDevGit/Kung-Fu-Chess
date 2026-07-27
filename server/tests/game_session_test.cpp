#include "tests/doctest.h"
#include "services/GameSession.h"
#include "services/GameSessionManager.h"
#include "persistence/Database.h"
#include "persistence/UserRepository.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace
{
GameSession::Player whitePlayer()
{
    return GameSession::Player{1, "alice", "conn-white"};
}

GameSession::Player blackPlayer()
{
    return GameSession::Player{2, "bob", "conn-black"};
}

// Real UserRepository backed by an in-memory SQLite DB -- GameSessionManager
// needs one to build the RatingService its GameOutcomeFn closes over (see
// GameSessionManager::createSession); tests don't care about the persisted
// rows, only that construction/forfeit rating calls don't crash.
struct TestUserRepository
{
    Database database{":memory:"};
    UserRepository users{database};
};
}

TEST_CASE("GameSession sends a GameStartedMessage to both participants as soon as it's constructed")
{
    std::vector<std::string> whiteSent;
    std::vector<std::string> blackSent;

    GameSession session(
        "test-session",
        whitePlayer(),
        blackPlayer(),
        [&](const std::string& connectionId, const std::string& json)
        {
            if (connectionId == "conn-white")
                whiteSent.push_back(json);
            else if (connectionId == "conn-black")
                blackSent.push_back(json);
        },
        [](int, int) {});

    REQUIRE(whiteSent.size() == 1);
    CHECK(whiteSent[0].find("\"type\":\"game_started\"") != std::string::npos);
    REQUIRE(blackSent.size() == 1);
    CHECK(blackSent[0].find("\"type\":\"game_started\"") != std::string::npos);
}

TEST_CASE("GameSession::requestMove rejects a request for the opponent's piece")
{
    GameSession session("test-session", whitePlayer(), blackPlayer(), [](const std::string&, const std::string&) {}, [](int, int) {});

    // Classic board: black pieces start on rows 0/1 (see GameFactory::createClassicBoard).
    // White's connection trying to move a black pawn must be rejected before
    // touching the engine at all.
    const GameSession::CommandOutcome outcome = session.requestMove("conn-white", Position(1, 4), Position(2, 4));

    CHECK(outcome.accepted == false);
    CHECK(outcome.reason == "not_your_piece");
}

TEST_CASE("GameSession::requestMove accepts a request for the requester's own piece")
{
    GameSession session("test-session", whitePlayer(), blackPlayer(), [](const std::string&, const std::string&) {}, [](int, int) {});

    const GameSession::CommandOutcome outcome = session.requestMove("conn-white", Position(6, 4), Position(5, 4));

    CHECK(outcome.accepted == true);
    CHECK(outcome.reason.empty());
}

TEST_CASE("GameSession::requestMove rejects an unknown connection")
{
    GameSession session("test-session", whitePlayer(), blackPlayer(), [](const std::string&, const std::string&) {}, [](int, int) {});

    const GameSession::CommandOutcome outcome = session.requestMove("conn-stranger", Position(6, 4), Position(5, 4));

    CHECK(outcome.accepted == false);
    CHECK(outcome.reason == "unknown_connection");
}

TEST_CASE("GameSession sends a GameView snapshot to both participants on every tick, and again when a move settles")
{
    std::vector<std::string> whiteSent;

    GameSession session(
        "test-session",
        whitePlayer(),
        blackPlayer(),
        [&](const std::string& connectionId, const std::string& json)
        {
            if (connectionId == "conn-white")
                whiteSent.push_back(json);
        },
        [](int, int) {});

    whiteSent.clear(); // drop the game_started push from construction

    session.requestMove("conn-white", Position(6, 4), Position(5, 4));
    CHECK(whiteSent.empty()); // requesting a move doesn't itself fire MoveExecutedEvent

    session.tick(1100); // MILLIS_PER_SQUARE (1000) plus margin -- settles the motion

    // Two pushes from this one tick: tick() itself pushes unconditionally
    // every time (keeps an in-flight motion's glide smooth), plus a second
    // one from MoveExecutedEvent firing because the motion happened to
    // settle during this same tick.
    REQUIRE(whiteSent.size() == 2);
    CHECK(whiteSent[0].find("\"type\":\"game_view\"") != std::string::npos);
    CHECK(whiteSent[1].find("\"type\":\"game_view\"") != std::string::npos);
}

TEST_CASE("GameSession::getGameView/isGameOver reflect this session's own state")
{
    GameSession session("test-session", whitePlayer(), blackPlayer(), [](const std::string&, const std::string&) {}, [](int, int) {});

    CHECK(session.isGameOver() == false);
    CHECK(session.getGameView().getBoard().getRows() == 8);
    CHECK(session.getGameView().getBoard().getCols() == 8);
}

TEST_CASE("An ordinary king-capture win fires GameOutcomeFn -- regression test for the gap where normal wins never updated score")
{
    // White's b1 knight (id 7, see GameFactory::createClassicBoard's per-column
    // id assignment) hops to the black king at (0,4) in four capture-ignoring
    // knight jumps, landing only on empty squares until the final,
    // king-capturing hop: (7,1)->(5,2)->(4,4)->(2,3)->(0,4). Knights ignore
    // blocking pieces (KnightRule only checks bounds/friendly-fire), so every
    // intermediate square just needs to be unoccupied, which rows 2-5 are in
    // the classic starting position.
    std::vector<std::pair<int, int>> gameOutcomeCalls;

    GameSession session(
        "test-session",
        whitePlayer(),
        blackPlayer(),
        [](const std::string&, const std::string&) {},
        [&](int winnerUserId, int loserUserId)
        {
            gameOutcomeCalls.emplace_back(winnerUserId, loserUserId);
        });

    const std::vector<std::pair<Position, Position>> hops = {
        {Position(7, 1), Position(5, 2)},
        {Position(5, 2), Position(4, 4)},
        {Position(4, 4), Position(2, 3)},
        {Position(2, 3), Position(0, 4)}, // captures the black king
    };

    for (std::size_t i = 0; i < hops.size(); ++i)
    {
        const GameSession::CommandOutcome outcome = session.requestMove("conn-white", hops[i].first, hops[i].second);
        CHECK(outcome.accepted == true);

        session.tick(1100); // MILLIS_PER_SQUARE (1000) plus margin -- settles the motion
        if (i + 1 < hops.size())
            session.tick(2100); // REST_DURATION_MILLIS (2000) plus margin -- this knight can move again
    }

    CHECK(session.isGameOver() == true);
    REQUIRE(gameOutcomeCalls.size() == 1);
    CHECK(gameOutcomeCalls[0].first == whitePlayer().userId); // winner
    CHECK(gameOutcomeCalls[0].second == blackPlayer().userId); // loser
}

TEST_CASE("GameSession stops sending to a disconnected participant and resumes after reconnect")
{
    std::vector<std::string> whiteSent;
    std::vector<std::string> blackSent;

    GameSession session(
        "test-session",
        whitePlayer(),
        blackPlayer(),
        [&](const std::string& connectionId, const std::string& json)
        {
            if (connectionId == "conn-white")
                whiteSent.push_back(json);
            else if (connectionId == "conn-black-2")
                blackSent.push_back(json);
            else if (connectionId == "conn-black")
                blackSent.push_back(json);
        },
        [](int, int) {});

    session.markDisconnected("conn-black");
    whiteSent.clear();
    blackSent.clear();

    session.tick(10); // black shouldn't receive anything while disconnected
    CHECK(whiteSent.size() == 1);
    CHECK(blackSent.empty());

    session.markReconnected(2, "conn-black-2");
    whiteSent.clear();
    blackSent.clear();

    session.tick(10);
    CHECK(whiteSent.size() == 1);
    CHECK(blackSent.size() >= 1); // opponent_reconnected (from markReconnected) already landed above; this is the tick's own push
}

TEST_CASE("GameSessionManager::createSession assigns distinct session ids and indexes both connections/users")
{
    TestUserRepository repo;
    GameSessionManager manager([](const std::string&, const std::string&) {}, repo.users);

    GameSession& session = manager.createSession(whitePlayer(), blackPlayer());

    CHECK(manager.findSessionByConnection("conn-white") == &session);
    CHECK(manager.findSessionByConnection("conn-black") == &session);
    CHECK(manager.findSessionByUserId(1) == &session);
    CHECK(manager.findSessionByUserId(2) == &session);
    CHECK(manager.findSessionByConnection("conn-stranger") == nullptr);
}

TEST_CASE("GameSessionManager::tickAll advances every session's clock")
{
    TestUserRepository repo;
    std::vector<std::string> sent;

    GameSessionManager manager(
        [&](const std::string&, const std::string& json)
        {
            sent.push_back(json);
        },
        repo.users);

    GameSession& session = manager.createSession(whitePlayer(), blackPlayer());
    session.requestMove("conn-white", Position(6, 4), Position(5, 4));
    sent.clear();

    manager.tickAll(1100);

    // Same reasoning as the GameSession-level test above: one unconditional
    // per-tick push (x2 participants) plus one more from the settling
    // MoveExecutedEvent (x2 participants) == 4 sends total.
    CHECK(sent.size() == 4);
}

TEST_CASE("GameSessionManager::onConnectionClosed marks the right session's participant disconnected and drops the connection route")
{
    TestUserRepository repo;
    std::vector<std::string> sent;

    GameSessionManager manager(
        [&](const std::string&, const std::string& json) { sent.push_back(json); },
        repo.users);

    manager.createSession(whitePlayer(), blackPlayer());
    sent.clear();

    manager.onConnectionClosed("conn-black");

    CHECK(manager.findSessionByConnection("conn-black") == nullptr);
    // opponent_disconnected went only to white (black's slot is disconnected).
    REQUIRE(sent.size() == 1);
    CHECK(sent[0].find("\"type\":\"opponent_disconnected\"") != std::string::npos);
}

TEST_CASE("A session does not forfeit immediately on disconnect -- only after the grace period elapses")
{
    // GameSession::tick() compares real wall-clock time (see MonotonicClock.h)
    // against MatchmakingConfig::RECONNECT_GRACE_MILLIS (30s), not the
    // milliseconds argument passed to tick()/tickAll() -- that argument only
    // advances the game engine's own logical clock. Actually elapsing the
    // real grace period isn't something a fast unit test should wait for;
    // this test instead guards against the more likely regression, a
    // forfeit firing immediately (or on the very next tick) after a drop.
    TestUserRepository repo;
    std::vector<std::string> sent;

    GameSessionManager manager(
        [&](const std::string&, const std::string& json) { sent.push_back(json); },
        repo.users);

    manager.createSession(whitePlayer(), blackPlayer());
    manager.onConnectionClosed("conn-black");
    sent.clear();

    manager.tickAll(10);

    const bool sawGameOver = std::any_of(sent.begin(), sent.end(), [](const std::string& json)
        { return json.find("\"type\":\"game_over\"") != std::string::npos; });
    CHECK(sawGameOver == false);
    CHECK(manager.findSessionByConnection("conn-white") != nullptr); // session still active
}

TEST_CASE("GameSessionManager::rebindConnection moves a session's connection index to the new connection")
{
    TestUserRepository repo;
    GameSessionManager manager([](const std::string&, const std::string&) {}, repo.users);

    GameSession& session = manager.createSession(whitePlayer(), blackPlayer());

    CHECK(manager.rebindConnection(2, "conn-black-2") == true);
    CHECK(manager.findSessionByConnection("conn-black-2") == &session);

    CHECK(manager.rebindConnection(999, "conn-nobody") == false);
}
