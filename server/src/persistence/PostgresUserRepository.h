#ifndef POSTGRES_USER_REPOSITORY_H
#define POSTGRES_USER_REPOSITORY_H

// See PostgresDatabase.h -- this whole file is a no-op unless
// KUNGFUCHESS_HAS_POSTGRES is defined (i.e. libpq was found at configure
// time and PostgresUserRepository was actually compiled/linked in).
#ifdef KUNGFUCHESS_HAS_POSTGRES

#include <mutex>
#include <string>

#include "IUserRepository.h"
#include "PostgresDatabase.h"

// Postgres-backed IUserRepository. Owns its own PostgresDatabase connection
// (pass a libpq connection string/URI) -- the only class in the codebase
// that knows libpqxx exists at all, besides PostgresDatabase; see
// IUserRepository.h for the contract this fulfills.
//
// Unlike SqliteUserRepository, this class guards every public method with
// its own mutex: a single pqxx::connection is not safe for concurrent use
// from multiple threads (SQLiteCpp serializes internally; libpqxx does
// not), and IUserRepository is called from both connection-handling threads
// (AuthService) and the server's own tick thread (RatingService) with no
// shared lock between them today.
class PostgresUserRepository : public IUserRepository
{
public:
    explicit PostgresUserRepository(const std::string& connectionString);

    UserRecord createUser(const std::string& username, const std::string& passwordHash) override;
    std::optional<UserRecord> findByUsername(const std::string& username) const override;
    std::optional<UserRecord> findById(int id) const override;
    void setScore(int userId, int newScore) override;

private:
    // mutable: locking/querying happen from const methods too (find*), same
    // reasoning as SqliteUserRepository's mutable Database member.
    mutable std::mutex mutex;
    mutable PostgresDatabase database;
};

#endif // KUNGFUCHESS_HAS_POSTGRES
#endif // POSTGRES_USER_REPOSITORY_H
