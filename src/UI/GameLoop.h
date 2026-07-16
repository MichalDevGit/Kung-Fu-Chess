#ifndef GAME_LOOP_H
#define GAME_LOOP_H

#include "BoardCanvas.h"
#include "Renderer.h"
#include "../logic/Controller/Controller.h"

class GameLoop
{
public:
    GameLoop(Controller& controller, Renderer& renderer, BoardCanvas& canvas);

    void run();

private:
    Controller& controller;
    Renderer& renderer;
    BoardCanvas& canvas;

    static void mouseCallback(int event, int x, int y, int flags, void* userdata);
    void onMouseDown(int x, int y);
};

#endif
