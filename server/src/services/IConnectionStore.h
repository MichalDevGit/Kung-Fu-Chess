#ifndef ICONNECTION_STORE_H
#define ICONNECTION_STORE_H

#include <optional>
#include <string>

#include "AuthenticatedUser.h"

// Raw storage primitives behind ConnectionRegistry -- pure CRUD, no business
// logic (the "a reconnect supersedes the old connection" dance stays in
// ConnectionRegistry itself, expressed in terms of these primitives). Mirrors
// the IUserRepository pattern: LocalConnectionStore is the in-memory default,
// RedisConnectionStore is the Redis-backed alternative, both behind this one
// interface so ConnectionRegistry never needs to know which is in use.
class IConnectionStore
{
public:
    virtual ~IConnectionStore() = default;

    virtual void set(const std::string& connectionId, const AuthenticatedUser& user) = 0;
    virtual void erase(const std::string& connectionId) = 0;
    virtual std::optional<AuthenticatedUser> find(const std::string& connectionId) const = 0;

    virtual void setUserConnection(int userId, const std::string& connectionId) = 0;
    virtual void eraseUserConnection(int userId) = 0;
    virtual std::optional<std::string> findConnectionForUser(int userId) const = 0;
};

#endif
