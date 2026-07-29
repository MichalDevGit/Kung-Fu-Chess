#ifndef LOCAL_RECONNECT_RESOLVER_H
#define LOCAL_RECONNECT_RESOLVER_H

#include "IReconnectResolver.h"

class GameSessionManager;

// The in-process implementation: calls straight into a real
// GameSessionManager& sitting in the same address space. This is what
// gamenode/'s own request router uses internally (GameSessionManager really
// does live there now), and what server/tests/auth_request_handler_test.cpp
// uses to exercise AuthRequestHandler without any networking involved --
// exactly the same behavior completeLogin had before this interface existed.
class LocalReconnectResolver : public IReconnectResolver
{
public:
    explicit LocalReconnectResolver(GameSessionManager& sessionManager);

    std::optional<protocol::MatchFoundResult> checkAndRebind(int userId, const std::string& newConnectionId) override;

private:
    GameSessionManager& sessionManager;
};

#endif
