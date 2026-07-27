#ifndef IUSER_REPOSITORY_H
#define IUSER_REPOSITORY_H

#include <optional>
#include <string>

#include "UserRecord.h"

// Uniform contract every user-persistence backend implements. Business logic
// (AuthService, RatingService, GameSessionManager, ...) depends only on this
// interface, never on a concrete backend -- see SqliteUserRepository and
// InMemoryUserRepository for the implementations, and RepositoryFactory for
// how one gets constructed.
class IUserRepository
{
public:
    virtual ~IUserRepository() = default;

    // Inserts a row with passwordHash stored exactly as given. Implementations
    // are deliberately ignorant of hashing -- they store whatever credential
    // string the caller hands them (see server/src/services/AuthService, the
    // only caller, which owns the PasswordHasher). New users start at
    // RatingConfig::INITIAL_RATING. Throws DuplicateUsernameException
    // (see UserRepositoryExceptions.h) if the username already exists.
    virtual UserRecord createUser(const std::string& username, const std::string& passwordHash) = 0;

    virtual std::optional<UserRecord> findByUsername(const std::string& username) const = 0;
    virtual std::optional<UserRecord> findById(int id) const = 0;

    // Sets score to an absolute value -- used by RatingService after
    // computing a post-game ELO rating (as opposed to a relative delta,
    // which nothing in this codebase needs anymore).
    virtual void setScore(int userId, int newScore) = 0;
};

#endif
