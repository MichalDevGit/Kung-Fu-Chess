#ifndef LOCAL_SESSION_INDEX_STORE_H
#define LOCAL_SESSION_INDEX_STORE_H

#include <mutex>
#include <unordered_map>

#include "ISessionIndexStore.h"

// In-memory ISessionIndexStore -- today's default. Ports the two
// unordered_maps GameSessionManager used to own directly, unchanged,
// including removeFinishedSessions' linear "erase every entry whose value
// equals sessionId" scan (unbindSession here) -- cheap for an in-memory map
// at this scale.
class LocalSessionIndexStore : public ISessionIndexStore
{
public:
    void bindSession(
        const std::string& sessionId, const std::vector<std::string>& connectionIds, const std::vector<int>& userIds) override;
    void bindConnection(const std::string& connectionId, const std::string& sessionId) override;
    void unbindConnection(const std::string& connectionId) override;
    void unbindSession(const std::string& sessionId) override;
    std::optional<std::string> findSessionIdByConnection(const std::string& connectionId) const override;
    std::optional<std::string> findSessionIdByUserId(int userId) const override;

private:
    mutable std::mutex mutex;
    std::unordered_map<std::string, std::string> sessionIdByConnection;
    std::unordered_map<int, std::string> sessionIdByUserId;
};

#endif
