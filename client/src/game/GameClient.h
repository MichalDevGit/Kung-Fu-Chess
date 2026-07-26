#ifndef GAME_CLIENT_H
#define GAME_CLIENT_H

#include <mutex>
#include <string>

#include "BoardMapper.h"
#include "../network/WebSocketClient.h"
#include "../../../common/PixelPosition.h"
#include "../../../common/DTO/GameView.h"

// Client-side replacement for holding a Controller& directly: GameLoop calls
// the exact same method surface (handlePixelClick/handlePixelJump/
// getGameView/isGameOver) it always has, but every call now goes over the
// network to the authoritative server instead of touching a local
// GameEngine. Registers itself as the WebSocketClient's message handler at
// construction (via WebSocketClient::setOnMessage -- the client must already
// exist and be connected by then) and immediately sends a JoinGameRequest.
//
// Owns the client-only BoardMapper: pixel math is a rendering concern the
// server has no reason to know about (see server/src/game/Controller, which
// dropped it in an earlier phase).
//
// click()'s old select-then-move UX (first click selects a same-color
// piece, second click on a different square either re-selects or commits a
// move) now has to live here instead of on the server: a networked
// MoveRequest already names both squares explicitly, so *something* has to
// decide when a second click resolves into one. The server no longer tracks
// per-controller selection state at all (see Controller::move), so this
// class reimplements that same interaction against the last-received
// BoardView/PieceView snapshot rather than a live Board/Piece -- the actual
// move legality is still solely the server's call; this only decides
// *when* to send a MoveRequest and with what from/to.
//
// latestView/hasSelection/selectedPosition/gameOver are all touched both
// from WebSocketClient's background message-callback thread (onMessage) and
// from the render loop's thread (handlePixelClick/getGameView/isGameOver),
// so every access is guarded by mutex -- same shared-mutable-state-across-
// threads situation CliShell's output mutex already handles, just for game
// state instead of console output.
class GameClient
{
public:
    explicit GameClient(WebSocketClient& client);

    void handlePixelClick(const PixelPosition& pixelPosition);
    void handlePixelJump(const PixelPosition& pixelPosition);

    GameView getGameView() const;
    bool isGameOver() const;

private:
    WebSocketClient& client;
    BoardMapper boardMapper;

    mutable std::mutex mutex;
    GameView latestView;
    bool hasSelection;
    PositionView selectedPosition;
    bool gameOver;

    void onMessage(const std::string& json);
};

#endif
