#include "services/AuthService.h"

#include <SQLiteCpp/SQLiteCpp.h>

#include "persistence/UserRecord.h"
#include "persistence/UserRepository.h"

AuthService::AuthService(UserRepository& users)
    : users(users)
{
}

auth::RegisterOutcome AuthService::registerUser(const std::string& username, const std::string& password)
{
    std::lock_guard<std::mutex> lock(mutex);

    try
    {
        const UserRecord created = users.createUser(username, password);
        return auth::RegisterOutcome{true, created.id, {}};
    }
    catch (const SQLite::Exception&)
    {
        // The UNIQUE constraint on username is the only expected failure mode here.
        return auth::RegisterOutcome{false, 0, "username_taken"};
    }
}

auth::LoginOutcome AuthService::login(const std::string& username, const std::string& password)
{
    std::lock_guard<std::mutex> lock(mutex);

    if (!users.verifyPassword(username, password))
        return auth::LoginOutcome{false, 0, 0, "invalid_credentials"};

    // verifyPassword just returned true, so the row is guaranteed to exist.
    const std::optional<UserRecord> user = users.findByUsername(username);
    return auth::LoginOutcome{true, user->id, user->score, {}};
}
