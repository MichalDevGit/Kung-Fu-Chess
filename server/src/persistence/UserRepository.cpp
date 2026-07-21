#include "UserRepository.h"

#include <SQLiteCpp/SQLiteCpp.h>

#include "Database.h"

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

UserRecord UserRepository::createUser(const std::string& username, const std::string& password)
{
    SQLite::Statement insert(database.raw(), "INSERT INTO users (username, password) VALUES (?, ?)");
    insert.bind(1, username);
    insert.bind(2, password);
    insert.exec();

    const int id = static_cast<int>(database.raw().getLastInsertRowid());
    return UserRecord{id, username, password, 0};
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

bool UserRepository::verifyPassword(const std::string& username, const std::string& password) const
{
    const std::optional<UserRecord> user = findByUsername(username);
    return user.has_value() && user->password == password;
}

void UserRepository::updateScore(int userId, int delta)
{
    SQLite::Statement update(database.raw(), "UPDATE users SET score = score + ? WHERE id = ?");
    update.bind(1, delta);
    update.bind(2, userId);
    update.exec();
}
