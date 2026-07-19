#pragma once
#include "BoardCanvas.h"
#include "../common/DTO/GameView.h"
#include "../common/PixelPosition.h"
#include "../common/enums/PieceState.h"

class AnimationFrame {
private:
    const BoardCanvas& canvas;

    static constexpr int JUMP_FRAME_COUNT = 5;

    static int frameIndexForProgress(double progress, int frameCount);

public:
    struct Resolution {
        PieceState state;
        int frame;
        PixelPosition position;
    };

    explicit AnimationFrame(const BoardCanvas& canvas);

    Resolution resolve(const GameView& gameView, int row, int col) const;
};
