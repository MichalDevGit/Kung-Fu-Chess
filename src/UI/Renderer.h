#pragma once
#include "BoardCanvas.h"
#include "SpriteManager.h"
#include "AnimationFrame.h"
#include "../common/DTO/GameView.h"

class Renderer {
private:
    BoardCanvas& canvas;
    SpriteManager& spriteManager;
    AnimationFrame& animationFrame;

public:
    Renderer(BoardCanvas& canvas, SpriteManager& spriteManager, AnimationFrame& animationFrame);
    void render(const GameView& gameView);
    void renderGameOver();
};