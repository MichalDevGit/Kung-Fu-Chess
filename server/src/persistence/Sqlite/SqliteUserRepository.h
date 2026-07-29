#ifndef SQLITE_USER_REPOSITORY_H
#define SQLITE_USER_REPOSITORY_H

#include <string>

#include "SqliteDatabase.h"
#include "../IUserRepository.h"

// SQLite-backed IUserRepository. Owns its own Database connection (pass a
// real file path for normal use, or ":memory:" for a disposable, per-test
// database) -- this is the only class in the codebase that knows SQLite
// exists; see IUserRepository.h for the contract this fulfills.
class SqliteUserRepository : public IUserRepository
{
public:
    explicit SqliteUserRepository(const std::string& dbPath);

    UserRecord createUser(const std::string& username, const std::string& passwordHash) override;
    std::optional<UserRecord> findByUsername(const std::string& username) const override;
    std::optional<UserRecord> findById(int id) const override;
    void setScore(int userId, int newScore) override;

private:
    // mutable: SQLite::Statement's constructor requires a non-const
    // Database& even for read-only queries (a quirk of the underlying C API,
    // not a logical constness violation) -- findByUsername/findById stay
    // const from IUserRepository's point of view, same as before this class
    // owned its Database by value instead of taking one by reference.
    mutable SqliteDatabase database;
};

#endif
