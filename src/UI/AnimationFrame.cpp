#include "AnimationFrame.h"
#include "../common/enums/RestKind.h"

#include <algorithm>

AnimationFrame::AnimationFrame(const BoardCanvas& canvas) : canvas(canvas) {}

int AnimationFrame::frameIndexForProgress(double progress, int frameCount) {
    double clamped = std::min(1.0, std::max(0.0, progress));
    int index = 1 + static_cast<int>(clamped * frameCount);
    return std::min(frameCount, index);  // non-looping: holds on the last frame
}

int AnimationFrame::frameIndexForLoop(long long currentTime, int frameCount, int framesPerSecond, int phaseOffset) {
    long long msPerFrame = 1000 / framesPerSecond;
    long long frameIndex = (currentTime / msPerFrame) + phaseOffset;
    return static_cast<int>(frameIndex % frameCount) + 1;  // looping: cycles forever
}

AnimationFrame::Resolution AnimationFrame::resolve(const GameView& gameView, int row, int col, int pieceId) const {
    const MotionView& motion = gameView.getMotion();
    if (motion.isActive() &&
        motion.getFrom().getRow() == row &&
        motion.getFrom().getCol() == col) {
        double progress = motion.getProgress(gameView.getCurrentTime());
        int frame = frameIndexForProgress(progress, MOVE_FRAME_COUNT);
        PixelPosition position = canvas.getInterpolatedPosition(motion.getFrom(), motion.getTo(), progress);
        return Resolution{PieceState::Moving, frame, position};
    }

    const JumpView& jump = gameView.getJump();
    if (jump.isActive() &&
        jump.getPosition().getRow() == row &&
        jump.getPosition().getCol() == col) {
        double progress = jump.getProgress(gameView.getCurrentTime());
        int frame = frameIndexForProgress(progress, JUMP_FRAME_COUNT);
        PixelPosition position = canvas.getCellPosition(row, col);
        return Resolution{PieceState::Jump, frame, position};
    }

    for (const RestView& rest : gameView.getRests()) {
        if (rest.getPosition().getRow() == row &&
            rest.getPosition().getCol() == col) {
            bool isShortRest = rest.getKind() == RestKind::Short;
            double progress = rest.getProgress(gameView.getCurrentTime());
            int frame = frameIndexForProgress(
                progress,
                isShortRest ? SHORT_REST_FRAME_COUNT : LONG_REST_FRAME_COUNT);
            PixelPosition position = canvas.getCellPosition(row, col);
            PieceState state = isShortRest ? PieceState::ShortRest : PieceState::LongRest;
            return Resolution{state, frame, position};
        }
    }

    PixelPosition position = canvas.getCellPosition(row, col);
    int frame = frameIndexForLoop(gameView.getCurrentTime(), IDLE_FRAME_COUNT, IDLE_FRAMES_PER_SECOND, pieceId);
    return Resolution{PieceState::Idle, frame, position};
}
