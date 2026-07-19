#include "Renderer.h"


#include "Renderer.h"

Renderer::Renderer(BoardCanvas& canvas, SpriteManager& spriteManager, AnimationFrame& animationFrame)
    : canvas(canvas), spriteManager(spriteManager), animationFrame(animationFrame) {}

void Renderer::render(const GameView& gameView) {

    canvas.beginFrame();

    const BoardView& board = gameView.getBoard();

    for (int row = 0; row < board.getRows(); ++row) {
        for (int col = 0; col < board.getCols(); ++col) {
            PieceView piece = board.getPiece(row, col);
            if (piece.getType() == PieceType::Empty) {
                continue;
            }

            AnimationFrame::Resolution resolution = animationFrame.resolve(gameView, row, col);
            Img sprite = spriteManager.getPieceSprite(piece, resolution.state, resolution.frame);
            canvas.drawPieceAtPixel(sprite, resolution.position);
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