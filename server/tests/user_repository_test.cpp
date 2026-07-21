#include "tests/doctest.h"

#include <SQLiteCpp/SQLiteCpp.h>

#include "persistence/Database.h"
#include "persistence/UserRepository.h"

TEST_CASE("Testing UserRepository")
{
    Database db(":memory:");
    UserRepository repo(db);

    SUBCASE("createUser then findByUsername/findById round-trip")
    {
        UserRecord created = repo.createUser("alice", "hunter2");
        CHECK(created.username == "alice");
        CHECK(created.password == "hunter2");
        CHECK(created.score == 0);

        std::optional<UserRecord> byUsername = repo.findByUsername("alice");
        REQUIRE(byUsername.has_value());
        CHECK(byUsername->id == created.id);

        std::optional<UserRecord> byId = repo.findById(created.id);
        REQUIRE(byId.has_value());
        CHECK(byId->username == "alice");
    }

    SUBCASE("findByUsername/findById return nullopt when missing")
    {
        CHECK_FALSE(repo.findByUsername("nobody").has_value());
        CHECK_FALSE(repo.findById(999).has_value());
    }

    SUBCASE("createUser rejects a duplicate username")
    {
        repo.createUser("bob", "pw1");
        CHECK_THROWS_AS(repo.createUser("bob", "pw2"), SQLite::Exception);
    }

    SUBCASE("verifyPassword")
    {
        repo.createUser("carol", "correct-password");
        CHECK(repo.verifyPassword("carol", "correct-password"));
        CHECK_FALSE(repo.verifyPassword("carol", "wrong-password"));
        CHECK_FALSE(repo.verifyPassword("nobody", "irrelevant"));
    }

    SUBCASE("updateScore applies a delta")
    {
        UserRecord user = repo.createUser("dave", "pw");
        repo.updateScore(user.id, 5);
        repo.updateScore(user.id, 3);

        std::optional<UserRecord> updated = repo.findById(user.id);
        REQUIRE(updated.has_value());
        CHECK(updated->score == 8);
    }
}
