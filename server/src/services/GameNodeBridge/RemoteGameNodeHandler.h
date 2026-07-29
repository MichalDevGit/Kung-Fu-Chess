#ifndef REMOTE_GAME_NODE_HANDLER_H
#define REMOTE_GAME_NODE_HANDLER_H

#include <string>

class GameNodeRequestPublisher;

// Gateway-side stand-in for MatchmakingRequestHandler/GameRequestHandler
// (MIGRATION_PLAN.md Phase 3) -- same handle(connectionId, rawJson) -> string
// shape, so server/src/main.cpp's dispatch() barely changes: everything
// AuthRequestHandler didn't recognize is simply forwarded to the Game Node
// instead of being handled in-process.
//
// Always returns "{}" synchronously (an inert, typeless JSON object --
// WebSocketServer::onMessage always sends the handler's return value back
// over the socket, so this can't return an empty string without sending an
// empty frame). No client-side code ever depends on that synchronous reply:
// GameClient never blocks on a move/jump response (it only reacts to the
// next game_view push), and waitForMatch only ever waits for match_found/
// no_match, never find_game's own synchronous ack -- both already arrived
// asynchronously even in the monolith (see MatchmakingRequestHandler's
// class comment). The real reply -- whatever MatchmakingRequestHandler/
// GameRequestHandler produce once the Game Node actually runs this request --
// comes back over GameNodeConfig::PUSHES_CHANNEL and reaches the client via
// GameNodePushRouter's default handler (WebSocketServer::sendTo), same as
// every other async push always has.
class RemoteGameNodeHandler
{
public:
    explicit RemoteGameNodeHandler(GameNodeRequestPublisher& publisher);

    std::string handle(const std::string& connectionId, const std::string& rawJson) const;

private:
    GameNodeRequestPublisher& publisher;
};

#endif
