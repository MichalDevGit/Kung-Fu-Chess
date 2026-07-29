#include "RedisConnectionStore.h"

namespace
{
    std::string connectionKey(const std::string& connectionId) { return "conn:" + connectionId; }
    std::string userConnectionKey(int userId) { return "user_conn:" + std::to_string(userId); }
}

RedisConnectionStore::RedisConnectionStore(sw::redis::Redis& redis) : redis(redis)
{
}

void RedisConnectionStore::set(const std::string& connectionId, const AuthenticatedUser& user)
{
    redis.set(connectionKey(connectionId), user.toJson().dump());
}

void RedisConnectionStore::erase(const std::string& connectionId)
{
    redis.del(connectionKey(connectionId));
}

std::optional<AuthenticatedUser> RedisConnectionStore::find(const std::string& connectionId) const
{
    const std::optional<std::string> value = redis.get(connectionKey(connectionId));
    if (!value.has_value())
        return std::nullopt;

    return AuthenticatedUser::fromJson(nlohmann::json::parse(*value));
}

void RedisConnectionStore::setUserConnection(int userId, const std::string& connectionId)
{
    redis.set(userConnectionKey(userId), connectionId);
}

void RedisConnectionStore::eraseUserConnection(int userId)
{
    redis.del(userConnectionKey(userId));
}

std::optional<std::string> RedisConnectionStore::findConnectionForUser(int userId) const
{
    return redis.get(userConnectionKey(userId));
}
