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

class MatchmakingRequestHandler;
class GameRequestHandler;
class Matchmaker;
class GameSessionManager;
class IReconnectResolver;
class GameNodePushPublisher;

// Game Node-side end of GameNodeConfig::REQUESTS_CHANNEL (MIGRATION_PLAN.md
// Phase 3) -- the process that actually owns Matchmaker/GameSessionManager
// now reacts to whatever the WebSocket Gateway forwards, instead of a
// WebSocketServer's request/close handlers calling into them directly.
// Three kinds of GameNodeRequest, same three things server/main.cpp used to
// do in-process:
//  - "client_message": runs the exact matchmaking-then-game dispatch
//    server/main.cpp's dispatch() used to run for anything past auth, then
//    publishes the reply via GameNodePushPublisher::push -- this is what
//    lets MatchmakingRequestHandler/GameRequestHandler themselves stay
//    completely unchanged (see server/tests/matchmaker_test.cpp,
//    game_request_handler_test.cpp -- still valid, still exercising these
//    same classes in place).
//  - "connection_closed": calls matchmaker.removeByConnection/
//    sessionManager.onConnectionClosed -- what WebSocketServer's close
//    handler used to do directly when it lived in the same process.
//  - "reconnect_check": answers via reconnectResolver.checkAndRebind and
//    publishes the result via GameNodePushPublisher::pushReconnectCheckResult
//    -- the Game Node side of RemoteReconnectResolver's blocking wait.
class GameNodeRequestRouter
{
public:
    GameNodeRequestRouter(
        sw::redis::Redis& redis,
        MatchmakingRequestHandler& matchmakingHandler,
        GameRequestHandler& gameHandler,
        Matchmaker& matchmaker,
        GameSessionManager& sessionManager,
        IReconnectResolver& reconnectResolver,
        GameNodePushPublisher& pushPublisher);

    // Subscribes and starts the background consume loop -- same
    // detached-forever shape as GameNodePushRouter::start()/the tick thread.
    void start();

private:
    MatchmakingRequestHandler& matchmakingHandler;
    GameRequestHandler& gameHandler;
    Matchmaker& matchmaker;
    GameSessionManager& sessionManager;
    IReconnectResolver& reconnectResolver;
    GameNodePushPublisher& pushPublisher;
    sw::redis::Redis& redis;

    void onMessage(const std::string& channel, const std::string& payload);
    std::string dispatchClientMessage(const std::string& connectionId, const std::string& rawJson) const;
};

#endif
