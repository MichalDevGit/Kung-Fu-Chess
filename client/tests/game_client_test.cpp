#include "tests/doctest.h"
#include "game/GameClient.h"
#include "network/WebSocketClient.h"
#include "common/DTO/GameView.h"
#include "common/DTO/BoardView.h"
#include "common/DTO/MotionView.h"
#include "common/DTO/JumpView.h"

namespace
{
GameView emptyView()
{
    BoardView board(8, 8, std::vector<PieceView>{});
    MotionView motion(false, PositionView(0, 0), PositionView(0, 0), 0, 0);
    JumpView jump(false, PositionView(0, 0), 0, 0);
    return GameView(board, motion, jump, std::vector<RestView>{}, false, PositionView(0, 0), 0);
}
}

// Deliberately never started/connected -- WebSocketClient documents that
// sending before the handshake completes silently drops the message, so
// handlePixelClick/handlePixelJump (which send Move/JumpRequests) are safe
// to exercise against a socket that never actually connects. The interesting
// "select a piece, then a second click sends a MoveRequest" branch depends
// on real board content -- verified manually end-to-end against a running
// server instead of here, since faking it would need a network-transport
// test seam this phase didn't call for.
TEST_CASE("GameClient starts from the GameView it's constructed with, no selection, no game-over")
{
    WebSocketClient client("ws://127.0.0.1:1", [](const std::string&) {});
    GameClient gameClient(client, emptyView());

    GameView view = gameClient.getGameView();

    CHECK(view.getHasSelection() == false);
    CHECK(gameClient.isGameOver() == false);
}

TEST_CASE("GameClient::handlePixelClick on an empty square selects nothing")
{
    WebSocketClient client("ws://127.0.0.1:1", [](const std::string&) {});
    GameClient gameClient(client, emptyView());

    gameClient.handlePixelClick(PixelPosition(50, 50));

    CHECK(gameClient.getGameView().getHasSelection() == false);
}

TEST_CASE("GameClient::handlePixelJump does not crash without a live connection")
{
    WebSocketClient client("ws://127.0.0.1:1", [](const std::string&) {});
    GameClient gameClient(client, emptyView());

    gameClient.handlePixelJump(PixelPosition(50, 50));

    CHECK(gameClient.isGameOver() == false);
}
