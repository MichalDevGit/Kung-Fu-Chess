#ifndef REDIS_SESSION_INDEX_STORE_H
#define REDIS_SESSION_INDEX_STORE_H

#include <sw/redis++/redis++.h>

#include "ISessionIndexStore.h"

// Redis-backed ISessionIndexStore. Keys: "session_conn:{connectionId}" /
// "session_user:{userId}" -> sessionId, plus an auxiliary Redis Set
// "session_keys:{sessionId}" (members = the exact Redis keys created for
// that session) maintained by bindSession/bindConnection -- this is what
// lets unbindSession clean up in O(session size) via SMEMBERS+DEL instead of
// a SCAN/KEYS over the whole keyspace (an anti-pattern in production Redis).
// This reverse-index bookkeeping is a private implementation detail of this
// class only; LocalSessionIndexStore doesn't need it.
class RedisSessionIndexStore : public ISessionIndexStore
{
public:
    explicit RedisSessionIndexStore(sw::redis::Redis& redis);

    void bindSession(
        const std::string& sessionId, const std::vector<std::string>& connectionIds, const std::vector<int>& userIds) override;
    void bindConnection(const std::string& connectionId, const std::string& sessionId) override;
    void unbindConnection(const std::string& connectionId) override;
    void unbindSession(const std::string& sessionId) override;
    std::optional<std::string> findSessionIdByConnection(const std::string& connectionId) const override;
    std::optional<std::string> findSessionIdByUserId(int userId) const override;

private:
    sw::redis::Redis& redis;

    void trackKeyForSession(const std::string& sessionId, const std::string& key);
};

#endif
