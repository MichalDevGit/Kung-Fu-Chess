#ifndef GAME_ALLOCATOR_REQUEST_ROUTER_H
#define GAME_ALLOCATOR_REQUEST_ROUTER_H

#include <string>

namespace sw
{
namespace redis
{
    class Redis;
}
}

class MatchmakingRequestHandler;
class Matchmaker;
class GameNodePushPublisher;

// The Game Allocator process's end of GameNodeConfig::
// MATCHMAKING_REQUESTS_CHANNEL (MIGRATION_PLAN.md Phase 4b) -- the
// find_game-handling half of what used to be GameNodeRequestRouter's job
// before this phase split gamenode/ into a singleton Allocator process and
// per-shard Game Server Shard processes (see GameNodeRequestRouter, now the
// shard-only half). Exactly one Allocator ever runs, so exactly one instance
// of this class ever subscribes to this channel.
//  - "client_message": runs MatchmakingRequestHandler::handle (only
//    find_game ever arrives here -- the Gateway sends move/jump straight to
//    whichever shard's channel owns the session instead), then publishes
//    the reply via GameNodePushPublisher::push.
//  - "connection_closed": calls matchmaker.removeByConnection -- a no-op if
//    this connection was never queued, exactly like Phase 3's version of
//    this check. (sessionManager.onConnectionClosed, Phase 3's other half of
//    this, is now GameNodeRequestRouter's job on whichever shard's channel
//    actually hosts the connection's session, if any -- see
//    GatewayGameRouter, which is what decides to send connection_closed to
//    both channels.)
class GameAllocatorRequestRouter
{
public:
    GameAllocatorRequestRouter(
        sw::redis::Redis& redis, MatchmakingRequestHandler& matchmakingHandler, Matchmaker& matchmaker, GameNodePushPublisher& pushPublisher);

    // Subscribes and starts the background consume loop -- same
    // detached-forever shape as GameNodePushRouter::start()/GameNodeRequestRouter::start().
    void start();

private:
    MatchmakingRequestHandler& matchmakingHandler;
    Matchmaker& matchmaker;
    GameNodePushPublisher& pushPublisher;
    sw::redis::Redis& redis;

    void onMessage(const std::string& channel, const std::string& payload);
};

#endif
