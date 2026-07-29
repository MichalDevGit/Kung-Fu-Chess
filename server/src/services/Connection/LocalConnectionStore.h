#ifndef LOCAL_CONNECTION_STORE_H
#define LOCAL_CONNECTION_STORE_H

#include <mutex>
#include <unordered_map>

#include "IConnectionStore.h"

// In-memory IConnectionStore -- today's default, and the only implementation
// until a Redis-backed deployment opts in (see RedisConnectionStore /
// server/src/main.cpp's KUNGFUCHESS_REDIS_URL wiring). Ports the two
// unordered_maps ConnectionRegistry used to own directly, unchanged.
class LocalConnectionStore : public IConnectionStore
{
public:
    void set(const std::string& connectionId, const AuthenticatedUser& user) override;
    void erase(const std::string& connectionId) override;
    std::optional<AuthenticatedUser> find(const std::string& connectionId) const override;

    void setUserConnection(int userId, const std::string& connectionId) override;
    void eraseUserConnection(int userId) override;
    std::optional<std::string> findConnectionForUser(int userId) const override;

private:
    mutable std::mutex mutex;
    std::unordered_map<std::string, AuthenticatedUser> byConnection;
    std::unordered_map<int, std::string> connectionByUserId;
};

#endif
