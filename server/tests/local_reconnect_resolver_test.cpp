#include "tests/doctest.h"

#include "persistence/InMemory/InMemoryUserRepository.h"
#include "services/GameNodeBridge/LocalReconnectResolver.h"
#include "services/GameSession/GameSessionManager.h"
#include "services/SessionIndex/LocalSessionIndexStore.h"

namespace
{
struct Fixture
{
    InMemoryUserRepository repo;
    LocalSessionIndexStore indexStore;
    GameSessionManager sessionManager{[](const std::string&, const std::string&) {}, repo, indexStore};
    LocalReconnectResolver resolver{sessionManager};
};
}

TEST_CASE("LocalReconnectResolver: a user with no active session gets nullopt")
{
    Fixture fixture;

    CHECK_FALSE(fixture.resolver.checkAndRebind(1, "new-conn").has_value());
}

TEST_CASE("LocalReconnectResolver: a user with an active session is rebound and returns a resume payload")
{
    Fixture fixture;

    GameSession& session = fixture.sessionManager.createSession(
        "test-session", GameSession::Player{1, "alice", "alice-old-conn"}, GameSession::Player{2, "bob", "bob-conn"});

    const std::optional<protocol::MatchFoundResult> resume = fixture.resolver.checkAndRebind(1, "alice-new-conn");

    REQUIRE(resume.has_value());
    CHECK(resume->sessionId == session.getId());
    CHECK(resume->color == PieceColor::White);
    CHECK(resume->opponentUsername == "bob");

    // The session's white slot is now also reachable via the new connection
    // id -- exactly what completeLogin needed
    // GameSessionManager::rebindConnection for before this interface
    // existed. (rebindConnection only adds this new route; the old
    // connectionId's own route is cleared separately, whenever that
    // connection's close event actually fires -- see
    // GameSessionManager::onConnectionClosed.)
    CHECK(fixture.sessionManager.findSessionByConnection("alice-new-conn") == &session);
}

TEST_CASE("LocalReconnectResolver: the other participant is unaffected by a reconnect check for their opponent")
{
    Fixture fixture;

    fixture.sessionManager.createSession(
        "test-session", GameSession::Player{1, "alice", "alice-conn"}, GameSession::Player{2, "bob", "bob-conn"});

    const std::optional<protocol::MatchFoundResult> resume = fixture.resolver.checkAndRebind(2, "bob-new-conn");

    REQUIRE(resume.has_value());
    CHECK(resume->color == PieceColor::Black);
    CHECK(resume->opponentUsername == "alice");
}
