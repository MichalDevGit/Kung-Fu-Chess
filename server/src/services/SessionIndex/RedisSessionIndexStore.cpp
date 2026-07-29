#include "RedisSessionIndexStore.h"

namespace
{
    std::string connectionKey(const std::string& connectionId) { return "session_conn:" + connectionId; }
    std::string userKey(int userId) { return "session_user:" + std::to_string(userId); }
    std::string sessionKeysKey(const std::string& sessionId) { return "session_keys:" + sessionId; }
}

RedisSessionIndexStore::RedisSessionIndexStore(sw::redis::Redis& redis) : redis(redis)
{
}

void RedisSessionIndexStore::trackKeyForSession(const std::string& sessionId, const std::string& key)
{
    redis.sadd(sessionKeysKey(sessionId), key);
}

void RedisSessionIndexStore::bindSession(
    const std::string& sessionId, const std::vector<std::string>& connectionIds, const std::vector<int>& userIds)
{
    for (const std::string& connectionId : connectionIds)
        bindConnection(connectionId, sessionId);

    for (int userId : userIds)
    {
        const std::string key = userKey(userId);
        redis.set(key, sessionId);
        trackKeyForSession(sessionId, key);
    }
}

void RedisSessionIndexStore::bindConnection(const std::string& connectionId, const std::string& sessionId)
{
    const std::string key = connectionKey(connectionId);
    redis.set(key, sessionId);
    trackKeyForSession(sessionId, key);
}

void RedisSessionIndexStore::unbindConnection(const std::string& connectionId)
{
    redis.del(connectionKey(connectionId));
}

void RedisSessionIndexStore::unbindSession(const std::string& sessionId)
{
    const std::string keysKey = sessionKeysKey(sessionId);

    std::vector<std::string> keys;
    redis.smembers(keysKey, std::back_inserter(keys));

    for (const std::string& key : keys)
        redis.del(key);

    redis.del(keysKey);
}

std::optional<std::string> RedisSessionIndexStore::findSessionIdByConnection(const std::string& connectionId) const
{
    return redis.get(connectionKey(connectionId));
}

std::optional<std::string> RedisSessionIndexStore::findSessionIdByUserId(int userId) const
{
    return redis.get(userKey(userId));
}
