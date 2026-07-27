#include "SqliteUserRepository.h"

#include <SQLiteCpp/SQLiteCpp.h>

#include "UserRepositoryExceptions.h"
#include "../../../common/Config/RatingConfig.h"

namespace
{
    UserRecord rowToRecord(SQLite::Statement& query)
    {
        return UserRecord{
            query.getColumn(0).getInt(),
            query.getColumn(1).getString(),
            query.getColumn(2).getString(),
            query.getColumn(3).getInt()};
    }
}

SqliteUserRepository::SqliteUserRepository(const std::string& dbPath)
    : database(dbPath)
{
}

UserRecord SqliteUserRepository::createUser(const std::string& username, const std::string& passwordHash)
{
    try
    {
        SQLite::Statement insert(database.raw(), "INSERT INTO users (username, password, score) VALUES (?, ?, ?)");
        insert.bind(1, username);
        insert.bind(2, passwordHash);
        insert.bind(3, RatingConfig::INITIAL_RATING);
        insert.exec();
    }
    catch (const SQLite::Exception&)
    {
        // The UNIQUE constraint on username is the only expected failure mode
        // here -- rethrown as a backend-agnostic exception so callers never
        // need to know SQLite is involved (see UserRepositoryExceptions.h).
        throw DuplicateUsernameException(username);
    }

    const int id = static_cast<int>(database.raw().getLastInsertRowid());
    return UserRecord{id, username, passwordHash, RatingConfig::INITIAL_RATING};
}

std::optional<UserRecord> SqliteUserRepository::findByUsername(const std::string& username) const
{
    SQLite::Statement query(database.raw(), "SELECT id, username, password, score FROM users WHERE username = ?");
    query.bind(1, username);

    if (!query.executeStep())
        return std::nullopt;

    return rowToRecord(query);
}

std::optional<UserRecord> SqliteUserRepository::findById(int id) const
{
    SQLite::Statement query(database.raw(), "SELECT id, username, password, score FROM users WHERE id = ?");
    query.bind(1, id);

    if (!query.executeStep())
        return std::nullopt;

    return rowToRecord(query);
}

void SqliteUserRepository::setScore(int userId, int newScore)
{
    SQLite::Statement update(database.raw(), "UPDATE users SET score = ? WHERE id = ?");
    update.bind(1, newScore);
    update.bind(2, userId);
    update.exec();
}
