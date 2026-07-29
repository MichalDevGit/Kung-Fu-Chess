#ifndef REDIS_CONNECTION_STORE_H
#define REDIS_CONNECTION_STORE_H

#include <sw/redis++/redis++.h>

#include "IConnectionStore.h"

// Redis-backed IConnectionStore. Keys: "conn:{connectionId}" -> JSON
// (AuthenticatedUser::toJson/fromJson), "user_conn:{userId}" -> plain
// connectionId string. Takes sw::redis::Redis& directly (no extra wrapper
// class -- it's already the thin, connection-pooled client redis-plus-plus
// provides); shared with RedisMatchQueueStore/RedisSessionIndexStore, all
// constructed from the same connection in server/src/main.cpp.
class RedisConnectionStore : public IConnectionStore
{
public:
    explicit RedisConnectionStore(sw::redis::Redis& redis);

    void set(const std::string& connectionId, const AuthenticatedUser& user) override;
    void erase(const std::string& connectionId) override;
    std::optional<AuthenticatedUser> find(const std::string& connectionId) const override;

    void setUserConnection(int userId, const std::string& connectionId) override;
    void eraseUserConnection(int userId) override;
    std::optional<std::string> findConnectionForUser(int userId) const override;

private:
    sw::redis::Redis& redis;
};

#endif
