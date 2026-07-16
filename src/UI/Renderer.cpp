#include "Renderer.h"


#include "Renderer.h"

Renderer::Renderer(BoardCanvas& canvas, SpriteManager& spriteManager)
    : canvas(canvas), spriteManager(spriteManager) {}

void Renderer::render(const BoardView& snapshot) {
    
    for (int row = 0; row < snapshot.getRows(); ++row) {
        for (int col = 0; col < snapshot.getCols(); ++col) {
            PieceView piece = snapshot.getPiece(row, col);
            if (piece.getType() != PieceType::Empty) {
                Img sprite = spriteManager.getPieceSprite(piece, PieceState::Idle, 1);
                canvas.drawPiece(sprite, row, col);
            }
        }
    }
    canvas.show();
}