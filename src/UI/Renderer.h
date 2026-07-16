#pragma once
#include "BoardCanvas.h"
#include "SpriteManager.h"
#include "../common/DTO/BoardView.h"

class Renderer {
private:
    BoardCanvas& canvas;
    SpriteManager& spriteManager;

public:
    Renderer(BoardCanvas& canvas, SpriteManager& spriteManager);
    void render(const BoardView& snapshot);
    void renderGameOver();
};