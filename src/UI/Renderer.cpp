#include "Renderer.h"


#include "Renderer.h"

Renderer::Renderer(BoardCanvas& canvas, SpriteManager& spriteManager)
    : canvas(canvas), spriteManager(spriteManager) {}

void Renderer::render(const GameView& gameView) {

    canvas.beginFrame();

    const BoardView& board = gameView.getBoard();
    const MotionView& motion = gameView.getMotion();

    for (int row = 0; row < board.getRows(); ++row) {
        for (int col = 0; col < board.getCols(); ++col) {
            if (motion.isActive() &&
                motion.getFrom().getRow() == row &&
                motion.getFrom().getCol() == col) {
                continue;  // drawn separately below, at its interpolated position
            }

            PieceView piece = board.getPiece(row, col);
            if (piece.getType() != PieceType::Empty) {
                Img sprite = spriteManager.getPieceSprite(piece, PieceState::Idle, 1);
                canvas.drawPiece(sprite, row, col);
            }
        }
    }

    if (motion.isActive()) {
        PieceView movingPiece = board.getPiece(motion.getFrom().getRow(), motion.getFrom().getCol());

        if (movingPiece.getType() != PieceType::Empty) {
            double progress = motion.getProgress(gameView.getCurrentTime());
            PixelPosition pixelPos = canvas.getInterpolatedPosition(motion.getFrom(), motion.getTo(), progress);
            Img sprite = spriteManager.getPieceSprite(movingPiece, PieceState::Idle, 1);
            canvas.drawPieceAtPixel(sprite, pixelPos);
        }
    }

    const JumpView& jump = gameView.getJump();
    if (jump.isActive()) {
        canvas.drawJumpHighlight(jump.getPosition().getRow(), jump.getPosition().getCol());
    }

    for (const RestView& rest : gameView.getRests()) {
        double progress = rest.getProgress(gameView.getCurrentTime());
        canvas.drawRestProgress(rest.getPosition().getRow(), rest.getPosition().getCol(), progress);
    }

    if (gameView.getHasSelection()) {
        canvas.drawSelectionHighlight(gameView.getSelectedPosition().getRow(), gameView.getSelectedPosition().getCol());
    }

    canvas.show();
}

void Renderer::renderGameOver(){
    canvas.drawText("Game Over");
}