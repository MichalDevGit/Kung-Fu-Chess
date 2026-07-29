#include "tests/doctest.h"

#include "persistence/Sqlite/SqliteUserRepository.h"
#include "persistence/UserRepositoryExceptions.h"
#include "../../common/Config/RatingConfig.h"

TEST_CASE("Testing SqliteUserRepository")
{
    SqliteUserRepository repo(":memory:");

    SUBCASE("createUser then findByUsername/findById round-trip")
    {
        // SqliteUserRepository stores whatever credential string it's given --
        // hashing is AuthService's job (see auth_service_test.cpp), so this
        // test passes a plain string through unchanged, same as any other
        // opaque field.
        UserRecord created = repo.createUser("alice", "already-hashed-value");
        CHECK(created.username == "alice");
        CHECK(created.password == "already-hashed-value");
        CHECK(created.score == RatingConfig::INITIAL_RATING);

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
        CHECK_THROWS_AS(repo.createUser("bob", "pw2"), DuplicateUsernameException);
    }

    SUBCASE("setScore sets an absolute value")
    {
        UserRecord user = repo.createUser("dave", "pw");
        repo.setScore(user.id, 1250);
        repo.setScore(user.id, 1235);

        std::optional<UserRecord> updated = repo.findById(user.id);
        REQUIRE(updated.has_value());
        CHECK(updated->score == 1235);
    }
}
