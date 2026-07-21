#include "tests/doctest.h"

#include "persistence/Database.h"
#include "persistence/UserRepository.h"
#include "services/AuthService.h"

TEST_CASE("Testing AuthService")
{
    Database db(":memory:");
    UserRepository repo(db);
    AuthService auth(repo);

    SUBCASE("registerUser succeeds for a new username")
    {
        const auth::RegisterOutcome outcome = auth.registerUser("alice", "hunter2");
        CHECK(outcome.success);
        CHECK(outcome.userId > 0);
        CHECK(outcome.error.empty());
    }

    SUBCASE("registerUser rejects a duplicate username")
    {
        auth.registerUser("bob", "pw1");
        const auth::RegisterOutcome outcome = auth.registerUser("bob", "pw2");
        CHECK_FALSE(outcome.success);
        CHECK(outcome.error == "username_taken");
    }

    SUBCASE("login succeeds with correct credentials")
    {
        const auth::RegisterOutcome registered = auth.registerUser("carol", "correct-password");
        const auth::LoginOutcome outcome = auth.login("carol", "correct-password");
        CHECK(outcome.success);
        CHECK(outcome.userId == registered.userId);
        CHECK(outcome.score == 0);
        CHECK(outcome.error.empty());
    }

    SUBCASE("login fails with wrong password")
    {
        auth.registerUser("dave", "correct-password");
        const auth::LoginOutcome outcome = auth.login("dave", "wrong-password");
        CHECK_FALSE(outcome.success);
        CHECK(outcome.error == "invalid_credentials");
    }

    SUBCASE("login fails for an unknown user")
    {
        const auth::LoginOutcome outcome = auth.login("nobody", "irrelevant");
        CHECK_FALSE(outcome.success);
        CHECK(outcome.error == "invalid_credentials");
    }
}
