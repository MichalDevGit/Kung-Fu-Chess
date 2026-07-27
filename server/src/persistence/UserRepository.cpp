#include "UserRepository.h"

#include <SQLiteCpp/SQLiteCpp.h>

#include "Database.h"
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

UserRepository::UserRepository(Database& database)
    : database(database)
{
}

UserRecord UserRepository::createUser(const std::string& username, const std::string& passwordHash)
{
    SQLite::Statement insert(database.raw(), "INSERT INTO users (username, password, score) VALUES (?, ?, ?)");
    insert.bind(1, username);
    insert.bind(2, passwordHash);
    insert.bind(3, RatingConfig::INITIAL_RATING);
    insert.exec();

    const int id = static_cast<int>(database.raw().getLastInsertRowid());
    return UserRecord{id, username, passwordHash, RatingConfig::INITIAL_RATING};
}

std::optional<UserRecord> UserRepository::findByUsername(const std::string& username) const
{
    SQLite::Statement query(database.raw(), "SELECT id, username, password, score FROM users WHERE username = ?");
    query.bind(1, username);

    if (!query.executeStep())
        return std::nullopt;

    return rowToRecord(query);
}

std::optional<UserRecord> UserRepository::findById(int id) const
{
    SQLite::Statement query(database.raw(), "SELECT id, username, password, score FROM users WHERE id = ?");
    query.bind(1, id);

    if (!query.executeStep())
        return std::nullopt;

    return rowToRecord(query);
}

void UserRepository::setScore(int userId, int newScore)
{
    SQLite::Statement update(database.raw(), "UPDATE users SET score = ? WHERE id = ?");
    update.bind(1, newScore);
    update.bind(2, userId);
    update.exec();
}
