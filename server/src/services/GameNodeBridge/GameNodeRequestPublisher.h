#ifndef GAME_NODE_REQUEST_PUBLISHER_H
#define GAME_NODE_REQUEST_PUBLISHER_H

#include <string>

namespace sw
{
namespace redis
{
    class Redis;
}
}

// Gateway-side end of GameNodeConfig::REQUESTS_CHANNEL (MIGRATION_PLAN.md
// Phase 3). Every method here is fire-and-forget (a plain Redis PUBLISH) --
// gamenode/'s GameNodeRequestRouter is the only thing that ever consumes
// these, and any reply it has comes back asynchronously over
// GameNodeConfig::PUSHES_CHANNEL (see GameNodePushRouter), never as this
// class's own return value.
class GameNodeRequestPublisher
{
public:
    explicit GameNodeRequestPublisher(sw::redis::Redis& redis);

    // Forwards a raw client request (find_game/move/jump, or anything else
    // AuthRequestHandler didn't recognize) exactly as it arrived --
    // gamenode/'s router runs the same matchmaking-then-game dispatch the
    // monolith used to run in-process.
    void forward(const std::string& connectionId, const std::string& rawJson);

    // connectionId's socket just closed -- lets gamenode's Matchmaker/
    // GameSessionManager react (removeByConnection/onConnectionClosed)
    // exactly as they did when they lived in the same process as
    // WebSocketServer's close handler.
    void notifyConnectionClosed(const std::string& connectionId);

    // Does userId already have an active session? See RemoteReconnectResolver
    // for the blocking wait on the reply this triggers.
    void requestReconnectCheck(const std::string& connectionId, int userId);

private:
    sw::redis::Redis& redis;
};

#endif
