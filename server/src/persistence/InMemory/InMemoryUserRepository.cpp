#include "InMemoryUserRepository.h"

#include "../UserRepositoryExceptions.h"
#include "../../../../common/Config/RatingConfig.h"

UserRecord InMemoryUserRepository::createUser(const std::string& username, const std::string& passwordHash)
{
    std::lock_guard<std::mutex> lock(mutex);

    for (const auto& [id, user] : usersById)
    {
        if (user.username == username)
            throw DuplicateUsernameException(username);
    }

    const int id = nextId++;
    const UserRecord record{id, username, passwordHash, RatingConfig::INITIAL_RATING};
    usersById.emplace(id, record);
    return record;
}

std::optional<UserRecord> InMemoryUserRepository::findByUsername(const std::string& username) const
{
    std::lock_guard<std::mutex> lock(mutex);

    for (const auto& [id, user] : usersById)
    {
        if (user.username == username)
            return user;
    }

    return std::nullopt;
}

std::optional<UserRecord> InMemoryUserRepository::findById(int id) const
{
    std::lock_guard<std::mutex> lock(mutex);

    const auto it = usersById.find(id);
    if (it == usersById.end())
        return std::nullopt;

    return it->second;
}

void InMemoryUserRepository::setScore(int userId, int newScore)
{
    std::lock_guard<std::mutex> lock(mutex);

    const auto it = usersById.find(userId);
    if (it != usersById.end())
        it->second.score = newScore;
}
