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

    // Inserts a row with the password stored as given (plain text for now --
    // hashing is a later, dedicated security step). Throws (propagates SQLite's
    // UNIQUE constraint violation) if the username already exists.
    UserRecord createUser(const std::string& username, const std::string& password);

    std::optional<UserRecord> findByUsername(const std::string& username) const;
    std::optional<UserRecord> findById(int id) const;

    // Placeholder until hashing is added -- kept as its own method now so callers
    // don't need to change when it is.
    bool verifyPassword(const std::string& username, const std::string& password) const;

    void updateScore(int userId, int delta);

private:
    Database& database;
};

#endif
