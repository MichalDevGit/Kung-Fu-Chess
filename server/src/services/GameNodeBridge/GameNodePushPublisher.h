#ifndef GAME_NODE_PUSH_PUBLISHER_H
#define GAME_NODE_PUSH_PUBLISHER_H

#include <string>

namespace sw
{
namespace redis
{
    class Redis;
}
}

// Game Node-side end of GameNodeConfig::PUSHES_CHANNEL (MIGRATION_PLAN.md
// Phase 3) -- the Redis-backed replacement for calling WebSocketServer::
// sendTo directly, since gamenode/ no longer holds any client connections
// itself. Used two ways in gamenode/src/main.cpp: as the SendToFn every
// GameSession gets (via GameSessionManager) for its own EventBus-driven
// pushes, and by GameNodeRequestRouter for a forwarded request's reply and
// for a reconnect-check's answer.
class GameNodePushPublisher
{
public:
    explicit GameNodePushPublisher(sw::redis::Redis& redis);

    // json is a raw protocol/ message string, relayed to connectionId
    // verbatim by the Gateway's GameNodePushRouter default handler.
    void push(const std::string& connectionId, const std::string& json);

    // matchFoundResultJson is a protocol::MatchFoundResult's toJson() if
    // userId had an active session, empty otherwise -- the reply to a
    // "reconnect_check" request (see GameNodeRequestRouter).
    void pushReconnectCheckResult(const std::string& connectionId, const std::string& matchFoundResultJson);

private:
    sw::redis::Redis& redis;
};

#endif
