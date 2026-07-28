#include "PostgresUserRepository.h"

#ifdef KUNGFUCHESS_HAS_POSTGRES

#include "UserRepositoryExceptions.h"
#include "../../../common/Config/RatingConfig.h"

namespace
{
    UserRecord rowToRecord(const pqxx::row& row)
    {
        return UserRecord{
            row[0].as<int>(),
            row[1].as<std::string>(),
            row[2].as<std::string>(),
            row[3].as<int>()};
    }
}

PostgresUserRepository::PostgresUserRepository(const std::string& connectionString)
    : database(connectionString)
{
}

UserRecord PostgresUserRepository::createUser(const std::string& username, const std::string& passwordHash)
{
    std::lock_guard<std::mutex> lock(mutex);

    try
    {
        pqxx::work txn(database.raw());
        pqxx::row inserted = txn.exec_params1(
            "INSERT INTO users (username, password, score) VALUES ($1, $2, $3) RETURNING id",
            username, passwordHash, RatingConfig::INITIAL_RATING);
        txn.commit();

        return UserRecord{inserted[0].as<int>(), username, passwordHash, RatingConfig::INITIAL_RATING};
    }
    catch (const pqxx::unique_violation&)
    {
        // The UNIQUE constraint on username is the only expected failure
        // mode here -- rethrown as a backend-agnostic exception so callers
        // never need to know Postgres is involved (see UserRepositoryExceptions.h).
        throw DuplicateUsernameException(username);
    }
}

std::optional<UserRecord> PostgresUserRepository::findByUsername(const std::string& username) const
{
    std::lock_guard<std::mutex> lock(mutex);

    pqxx::work txn(database.raw());
    pqxx::result result = txn.exec_params(
        "SELECT id, username, password, score FROM users WHERE username = $1", username);

    if (result.empty())
        return std::nullopt;

    return rowToRecord(result[0]);
}

std::optional<UserRecord> PostgresUserRepository::findById(int id) const
{
    std::lock_guard<std::mutex> lock(mutex);

    pqxx::work txn(database.raw());
    pqxx::result result = txn.exec_params(
        "SELECT id, username, password, score FROM users WHERE id = $1", id);

    if (result.empty())
        return std::nullopt;

    return rowToRecord(result[0]);
}

void PostgresUserRepository::setScore(int userId, int newScore)
{
    std::lock_guard<std::mutex> lock(mutex);

    pqxx::work txn(database.raw());
    txn.exec_params("UPDATE users SET score = $1 WHERE id = $2", newScore, userId);
    txn.commit();
}

#endif // KUNGFUCHESS_HAS_POSTGRES
