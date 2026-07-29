#ifndef REMOTE_RECONNECT_RESOLVER_H
#define REMOTE_RECONNECT_RESOLVER_H

#include "IReconnectResolver.h"

class GameNodePushRouter;
class GameNodeRequestPublisher;

// The real, cross-process implementation used by the WebSocket Gateway
// (MIGRATION_PLAN.md Phase 3): publishes a "reconnect_check" request to the
// Game Node and blocks (with a timeout -- GameNodeConfig::
// RECONNECT_CHECK_TIMEOUT_MILLIS) for its answer over
// GameNodeConfig::PUSHES_CHANNEL. This is the one place this phase keeps a
// synchronous cross-process round trip -- see the class comment on
// IReconnectResolver for why AuthRequestHandler's ordering guarantee (the
// resume push must arrive before login_result) requires it. A timeout is
// treated the same as "no active session" -- a returning player just falls
// through to ordinary matchmaking instead of resuming, rather than the whole
// login hanging if the Game Node is briefly unreachable.
class RemoteReconnectResolver : public IReconnectResolver
{
public:
    RemoteReconnectResolver(GameNodePushRouter& pushRouter, GameNodeRequestPublisher& requestPublisher);

    std::optional<protocol::MatchFoundResult> checkAndRebind(int userId, const std::string& newConnectionId) override;

private:
    GameNodePushRouter& pushRouter;
    GameNodeRequestPublisher& requestPublisher;
};

#endif
