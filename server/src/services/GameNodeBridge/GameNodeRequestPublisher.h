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

// Sender-side end of every Gateway/Game Allocator -> {Game Allocator, Game
// Server Shard} request (MIGRATION_PLAN.md Phase 3, reshaped by Phase 4b).
// Every method here is fire-and-forget (a plain Redis PUBLISH) -- whatever
// consumes a given channel (GameAllocatorRequestRouter for
// GameNodeConfig::MATCHMAKING_REQUESTS_CHANNEL, GameNodeRequestRouter for a
// specific shard's GameNodeConfig::shardRequestsChannel(shardId)) replies
// asynchronously over GameNodeConfig::PUSHES_CHANNEL instead, never as this
// class's own return value.
//
// Phase 4b: every method now takes an explicit `channel` instead of always
// publishing to one hardcoded constant -- there is no longer one single
// request channel now that a Gateway must address either the one Allocator
// or a specific shard, and a Game Allocator must address whichever shard it
// just picked.
class GameNodeRequestPublisher
{
public:
    explicit GameNodeRequestPublisher(sw::redis::Redis& redis);

    // Forwards a raw client request (find_game/move/jump) exactly as it
    // arrived -- the receiving process runs the same matchmaking or game
    // dispatch the monolith used to run in-process.
    void forward(const std::string& channel, const std::string& connectionId, const std::string& rawJson);

    // connectionId's socket just closed -- lets a Matchmaker (on
    // GameNodeConfig::MATCHMAKING_REQUESTS_CHANNEL) or a GameSessionManager
    // (on a specific shard's channel) react (removeByConnection/
    // onConnectionClosed) exactly as they did when they lived in the same
    // process as WebSocketServer's close handler.
    void notifyConnectionClosed(const std::string& channel, const std::string& connectionId);

    // Does userId already have an active session? Sent only to the specific
    // shard IGameShardRoutingStore says hosts it -- see RemoteReconnectResolver
    // for the blocking wait on the reply this triggers.
    void requestReconnectCheck(const std::string& channel, const std::string& connectionId, int userId);

    // Fire-and-forget "create this session, with this pre-assigned id, on
    // your shard" -- the Game Allocator's side of GameNodeCreateSessionRequest
    // (MIGRATION_PLAN.md Phase 4b). No reply is awaited; see GameAllocator's
    // class comment for why one isn't needed.
    void requestSessionCreation(
        const std::string& channel,
        const std::string& sessionId,
        int whiteUserId,
        const std::string& whiteUsername,
        const std::string& whiteConnectionId,
        int blackUserId,
        const std::string& blackUsername,
        const std::string& blackConnectionId);

private:
    sw::redis::Redis& redis;
};

#endif
