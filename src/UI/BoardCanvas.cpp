#include "BoardCanvas.h"

BoardCanvas::BoardCanvas(const std::string& boardPath, int cellSize) 
    : boardImage(boardPath), cellSize(cellSize) {}

void BoardCanvas::drawPiece(Img& piece, int row, int col) {
    PixelPosition pos = getCellPosition(row, col);
    piece.draw_on(boardImage, pos.getX(), pos.getY());
}

PixelPosition BoardCanvas::getCellPosition(int row, int col) const {
    return PixelPosition(col * cellSize, row * cellSize);
}

void BoardCanvas::show() { boardImage.show(); }