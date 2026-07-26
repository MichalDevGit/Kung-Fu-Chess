#ifndef GAME_LOOP_H
#define GAME_LOOP_H

#include "BoardCanvas.h"
#include "Renderer.h"
#include "../game/GameClient.h"

// The live loop and the only place in client/ that touches input directly.
// Held reference changed from the old in-process Controller& to GameClient&
// -- the method surface it calls (handlePixelClick/handlePixelJump/
// getGameView/isGameOver) is otherwise identical, so nothing else about this
// class changes. No longer measures a wall-clock delta / calls a per-frame
// "wait": the authoritative clock now advances on the server's own tick
// loop, independent of this render loop.
class GameLoop
{
public:
    GameLoop(GameClient& gameClient, Renderer& renderer, BoardCanvas& canvas);

    void run();

private:
    GameClient& gameClient;
    Renderer& renderer;
    BoardCanvas& canvas;

    static void mouseCallback(int event, int x, int y, int flags, void* userdata);
    void onMouseDown(int x, int y);
    void onMouseRightDown(int x, int y);
};

#endif
