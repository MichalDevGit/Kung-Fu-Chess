#ifndef GAME_NODE_REQUEST_ROUTER_H
#define GAME_NODE_REQUEST_ROUTER_H

#include <string>

namespace sw
{
namespace redis
{
    class Redis;
}
}

class GameRequestHandler;
class GameSessionManager;
class IReconnectResolver;
class GameNodePushPublisher;

// A specific Game Server Shard's end of its own
// GameNodeConfig::shardRequestsChannel(shardId) (MIGRATION_PLAN.md Phase 3,
// reshaped by Phase 4b once a shard stopped also being the one place
// Matchmaker/MatchmakingRequestHandler lived -- see GameAllocatorRequestRouter
// for that half now). Four kinds of request ever arrive on this channel:
//  - "create_session": the Game Allocator's fire-and-forget instruction to
//    actually construct the GameSession it already decided (elsewhere) to
//    route here -- calls sessionManager.createSession with the given,
//    pre-assigned sessionId. No reply is published; the Allocator doesn't
//    wait for one (see GameAllocator's class comment for why).
//  - "client_message": runs GameRequestHandler's move/jump dispatch, then
//    publishes the reply via GameNodePushPublisher::push -- what lets
//    GameRequestHandler itself stay completely unchanged (see
//    server/tests/game_request_handler_test.cpp -- still valid, still
//    exercising this same class in place). find_game never arrives here
//    (MIGRATION_PLAN.md Phase 4b: the Gateway sends that to the Allocator's
//    channel instead), so there's no matchmaking-then-game fallback dispatch
//    to run here anymore, unlike Phase 3's version of this class.
//  - "connection_closed": calls sessionManager.onConnectionClosed -- what
//    WebSocketServer's close handler used to do directly when it lived in
//    the same process. (matchmaker.removeByConnection, Phase 3's other half
//    of this, is now GameAllocatorRequestRouter's job on its own channel.)
//  - "reconnect_check": answers via reconnectResolver.checkAndRebind and
//    publishes the result via GameNodePushPublisher::pushReconnectCheckResult
//    -- the shard side of RemoteReconnectResolver's blocking wait. Only ever
//    arrives here because the Gateway already resolved, via
//    IGameShardRoutingStore, that *this* shard is the one hosting userId's
//    session -- see RemoteReconnectResolver.
class GameNodeRequestRouter
{
public:
    GameNodeRequestRouter(
        sw::redis::Redis& redis,
        std::string channel,
        GameRequestHandler& gameHandler,
        GameSessionManager& sessionManager,
        IReconnectResolver& reconnectResolver,
        GameNodePushPublisher& pushPublisher);

    // Subscribes and starts the background consume loop -- same
    // detached-forever shape as GameNodePushRouter::start()/the tick thread.
    void start();

private:
    GameRequestHandler& gameHandler;
    GameSessionManager& sessionManager;
    IReconnectResolver& reconnectResolver;
    GameNodePushPublisher& pushPublisher;
    sw::redis::Redis& redis;
    std::string channel;

    void onMessage(const std::string& channel, const std::string& payload);
};

#endif
