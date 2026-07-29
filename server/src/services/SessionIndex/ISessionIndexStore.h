#ifndef ISESSION_INDEX_STORE_H
#define ISESSION_INDEX_STORE_H

#include <optional>
#include <string>
#include <vector>

// Raw storage behind GameSessionManager's two lookup indexes (connectionId
// -> sessionId, userId -> sessionId) only -- never the live `sessions` map
// of real GameSession objects, which stays in-process only (a GameEngine
// can't be externalized without a much bigger redesign reserved for a later
// phase). Mirrors the IUserRepository pattern: LocalSessionIndexStore is the
// in-memory default, RedisSessionIndexStore is the Redis-backed alternative.
class ISessionIndexStore
{
public:
    virtual ~ISessionIndexStore() = default;

    // Associates sessionId with every given connection/user id at once --
    // called once, from GameSessionManager::createSession.
    virtual void bindSession(
        const std::string& sessionId, const std::vector<std::string>& connectionIds, const std::vector<int>& userIds) = 0;

    // Points an additional connectionId at an already-bound session --
    // GameSessionManager::rebindConnection's reconnect case.
    virtual void bindConnection(const std::string& connectionId, const std::string& sessionId) = 0;

    // Removes only the connection index entry -- the userId index, and the
    // session itself, are untouched. GameSessionManager::onConnectionClosed.
    virtual void unbindConnection(const std::string& connectionId) = 0;

    // Removes every connection/user index entry associated with sessionId.
    // GameSessionManager::removeFinishedSessions.
    virtual void unbindSession(const std::string& sessionId) = 0;

    virtual std::optional<std::string> findSessionIdByConnection(const std::string& connectionId) const = 0;
    virtual std::optional<std::string> findSessionIdByUserId(int userId) const = 0;
};

#endif
