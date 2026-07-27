#ifndef USER_REPOSITORY_H
#define USER_REPOSITORY_H

#include <optional>
#include <string>

#include "UserRecord.h"

class Database;

class UserRepository
{
public:
    explicit UserRepository(Database& database);

    // Inserts a row with passwordHash stored exactly as given. This layer is
    // deliberately ignorant of hashing -- it stores whatever credential
    // string the caller hands it (see server/src/services/AuthService, the
    // only caller, which owns the PasswordHasher). New users start at
    // RatingConfig::INITIAL_RATING. Throws (propagates SQLite's UNIQUE
    // constraint violation) if the username already exists.
    UserRecord createUser(const std::string& username, const std::string& passwordHash);

    std::optional<UserRecord> findByUsername(const std::string& username) const;
    std::optional<UserRecord> findById(int id) const;

    // Sets score to an absolute value -- used by RatingService after
    // computing a post-game ELO rating (as opposed to a relative delta,
    // which nothing in this codebase needs anymore).
    void setScore(int userId, int newScore);

private:
    Database& database;
};

#endif
