#ifndef GATEWAY_GAME_ROUTER_H
#define GATEWAY_GAME_ROUTER_H

#include <optional>
#include <string>

class GameNodeRequestPublisher;
class ISessionIndexStore;
class IGameShardRoutingStore;

// Gateway-side stand-in for MatchmakingRequestHandler/GameRequestHandler
// (MIGRATION_PLAN.md Phase 3), reshaped by Phase 4b to actually pick which
// process a request goes to instead of forwarding everything blindly to one
// Game Node: find_game always goes to the one Game Allocator; move/jump are
// routed to whichever shard IGameShardRoutingStore (via the session
// connectionId belongs to, looked up through ISessionIndexStore) says
// currently hosts the session. Both stores are read-only from this class's
// point of view -- writing them is GameAllocator's job.
//
// handle() always returns "{}" synchronously for find_game/move/jump (an
// inert, typeless JSON object -- WebSocketServer::onMessage always sends the
// handler's return value back over the socket, so this can't return an
// empty string without sending an empty frame). No client-side code ever
// depends on that synchronous reply: GameClient never blocks on a move/jump
// response (it only reacts to the next game_view push), and waitForMatch
// only ever waits for match_found/no_match, never find_game's own
// synchronous ack -- both already arrived asynchronously even in the
// monolith. The real reply -- whatever MatchmakingRequestHandler/
// GameRequestHandler produce once the right process actually runs this
// request -- comes back over GameNodeConfig::PUSHES_CHANNEL and reaches the
// client via GameNodePushRouter's default handler (WebSocketServer::sendTo),
// same as every other async push always has.
class GatewayGameRouter
{
public:
    GatewayGameRouter(GameNodeRequestPublisher& publisher, ISessionIndexStore& sessionIndexStore, IGameShardRoutingStore& shardRoutingStore);

    std::string handle(const std::string& connectionId, const std::string& rawJson) const;

    // connectionId's socket just closed -- notified to the Allocator
    // unconditionally (removeByConnection is a no-op if it was never queued)
    // and, separately, to whichever shard (if any) is currently hosting its
    // session.
    void notifyConnectionClosed(const std::string& connectionId) const;

private:
    GameNodeRequestPublisher& publisher;
    ISessionIndexStore& sessionIndexStore;
    IGameShardRoutingStore& shardRoutingStore;

    std::optional<std::string> shardChannelForConnection(const std::string& connectionId) const;
};

#endif
