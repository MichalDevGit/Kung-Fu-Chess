#pragma once

#include "BoardCanvas.h"
#include "SpriteManager.h"
#include "../common/DTO/BoardView.h"


class Renderer {
private:
    BoardCanvas& canvas;
    SpriteManager& sprite_manager;


    void draw_board(const BoardView& snapshot);


public:
    Renderer(BoardCanvas& canvas,
             SpriteManager& sprite_manager);


    void render(const BoardView& snapshot);
};