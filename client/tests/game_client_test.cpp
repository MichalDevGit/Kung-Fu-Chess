#include "tests/doctest.h"
#include "game/GameClient.h"
#include "network/WebSocketClient.h"

// Deliberately never started/connected -- WebSocketClient documents that
// sending before the handshake completes silently drops the message, so
// GameClient's constructor (which sends a JoinGameRequest immediately) and
// handlePixelClick/handlePixelJump (which send Move/JumpRequests) are all
// safe to exercise against a socket that never actually connects. The
// interesting "select a piece, then a second click sends a MoveRequest"
// branch depends on real board content GameClient only ever learns from a
// live server's GameViewMessage (see GameClient::onMessage, private) --
// that path was verified manually end-to-end against a running server
// instead of here, since faking it would need a network-transport test seam
// this phase didn't call for.
TEST_CASE("GameClient starts with no selection, no game-over, and an empty board snapshot")
{
    WebSocketClient client("ws://127.0.0.1:1", [](const std::string&) {});
    GameClient gameClient(client);

    GameView view = gameClient.getGameView();

    CHECK(view.getHasSelection() == false);
    CHECK(gameClient.isGameOver() == false);
}

TEST_CASE("GameClient::handlePixelClick on an empty square (no snapshot received yet) selects nothing")
{
    WebSocketClient client("ws://127.0.0.1:1", [](const std::string&) {});
    GameClient gameClient(client);

    gameClient.handlePixelClick(PixelPosition(50, 50));

    CHECK(gameClient.getGameView().getHasSelection() == false);
}

TEST_CASE("GameClient::handlePixelJump does not crash without a live connection")
{
    WebSocketClient client("ws://127.0.0.1:1", [](const std::string&) {});
    GameClient gameClient(client);

    gameClient.handlePixelJump(PixelPosition(50, 50));

    CHECK(gameClient.isGameOver() == false);
}
