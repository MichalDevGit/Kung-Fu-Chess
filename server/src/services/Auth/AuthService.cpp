#include "services/Auth/AuthService.h"

#include "persistence/IUserRepository.h"
#include "persistence/UserRecord.h"
#include "persistence/UserRepositoryExceptions.h"

AuthService::AuthService(IUserRepository& users)
    : users(users)
{
}

auth::RegisterOutcome AuthService::registerUser(const std::string& username, const std::string& password)
{
    std::lock_guard<std::mutex> lock(mutex);

    try
    {
        const UserRecord created = users.createUser(username, hasher.hash(password));
        return auth::RegisterOutcome{true, created.id, {}};
    }
    catch (const DuplicateUsernameException&)
    {
        return auth::RegisterOutcome{false, 0, "username_taken"};
    }
}

auth::LoginOutcome AuthService::login(const std::string& username, const std::string& password)
{
    std::lock_guard<std::mutex> lock(mutex);

    const std::optional<UserRecord> user = users.findByUsername(username);
    if (!user.has_value() || !hasher.verify(password, user->password))
        return auth::LoginOutcome{false, 0, 0, "invalid_credentials"};

    return auth::LoginOutcome{true, user->id, user->score, {}};
}
