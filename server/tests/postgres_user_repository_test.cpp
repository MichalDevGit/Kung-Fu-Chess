// Only compiled (as a real translation unit) in a build where libpq was
// found and PostgresUserRepository was actually linked in -- see
// server/CMakeLists.txt / KUNGFUCHESS_HAS_POSTGRES. Requires a real,
// reachable Postgres (docker-compose's `postgres` service, by default) --
// unlike SqliteUserRepository's disposable ":memory:", there's no per-test
// throwaway database, so each TEST_CASE truncates `users` up front to stay
// independent across runs against a persistent instance.
#ifdef KUNGFUCHESS_HAS_POSTGRES

#include "tests/doctest.h"

#include <cstdlib>
#include <string>

#include <pqxx/pqxx>

#include "persistence/PostgresUserRepository.h"
#include "persistence/UserRepositoryExceptions.h"
#include "../../common/Config/RatingConfig.h"

namespace
{
    std::string testConnectionString()
    {
        const char* url = std::getenv("KUNGFUCHESS_TEST_POSTGRES_URL");
        return url ? std::string(url) : "postgresql://kungfuchess:kungfuchess@localhost:5432/kungfuchess";
    }

    void resetUsersTable(const std::string& connectionString)
    {
        pqxx::connection conn(connectionString);
        pqxx::work txn(conn);
        txn.exec("CREATE TABLE IF NOT EXISTS users ("
                 "  id         SERIAL PRIMARY KEY,"
                 "  username   TEXT UNIQUE NOT NULL,"
                 "  password   TEXT NOT NULL,"
                 "  score      INTEGER NOT NULL DEFAULT 0,"
                 "  created_at TIMESTAMPTZ DEFAULT now()"
                 ")");
        txn.exec("TRUNCATE TABLE users RESTART IDENTITY");
        txn.commit();
    }
}

TEST_CASE("Testing PostgresUserRepository")
{
    const std::string connectionString = testConnectionString();
    resetUsersTable(connectionString);
    PostgresUserRepository repo(connectionString);

    SUBCASE("createUser then findByUsername/findById round-trip")
    {
        // PostgresUserRepository stores whatever credential string it's
        // given -- hashing is AuthService's job, same contract as
        // SqliteUserRepository (see sqlite_user_repository_test.cpp).
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

#endif // KUNGFUCHESS_HAS_POSTGRES
