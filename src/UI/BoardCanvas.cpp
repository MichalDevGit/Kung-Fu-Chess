#include "BoardCanvas.h"

BoardCanvas::BoardCanvas(const std::string& boardPath, int cellSize)
    : background(boardPath), frame(background), cellSize(cellSize) {}

void BoardCanvas::beginFrame() {
    frame = background;
}

void BoardCanvas::drawPiece(Img& piece, int row, int col) {
    PixelPosition pos = getCellPosition(row, col);
    piece.draw_on(frame, pos.getX(), pos.getY());
}

PixelPosition BoardCanvas::getCellPosition(int row, int col) const {
    return PixelPosition(col * cellSize, row * cellSize);
}

void BoardCanvas::show() { frame.show(); }

const std::string& BoardCanvas::getWindowName() const { return Img::windowName(); }