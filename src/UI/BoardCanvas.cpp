#include "BoardCanvas.h"

#include <algorithm>

const cv::Scalar BoardCanvas::SELECTION_COLOR(0, 255, 255, 255);  // yellow
const cv::Scalar BoardCanvas::JUMP_COLOR(255, 0, 255, 255);       // magenta
const cv::Scalar BoardCanvas::REST_BAR_COLOR(0, 165, 255, 255);   // orange

BoardCanvas::BoardCanvas(const std::string& boardPath, int cellSize)
    : background(boardPath), frame(background), cellSize(cellSize) {}

void BoardCanvas::beginFrame() {
    frame = background;
}

void BoardCanvas::drawPieceAtPixel(Img& piece, const PixelPosition& pixelPos) {
    piece.draw_on(frame, pixelPos.getX(), pixelPos.getY());
}

void BoardCanvas::drawPiece(Img& piece, int row, int col) {
    drawPieceAtPixel(piece, getCellPosition(row, col));
}

PixelPosition BoardCanvas::getCellPosition(int row, int col) const {
    return PixelPosition(col * cellSize, row * cellSize);
}

PixelPosition BoardCanvas::getInterpolatedPosition(const PositionView& from, const PositionView& to, double progress) const {
    PixelPosition fromPixel = getCellPosition(from.getRow(), from.getCol());
    PixelPosition toPixel = getCellPosition(to.getRow(), to.getCol());

    int x = fromPixel.getX() + static_cast<int>((toPixel.getX() - fromPixel.getX()) * progress);
    int y = fromPixel.getY() + static_cast<int>((toPixel.getY() - fromPixel.getY()) * progress);

    return PixelPosition(x, y);
}

void BoardCanvas::drawCellOutline(int row, int col, const cv::Scalar& color, int thickness) {
    PixelPosition pos = getCellPosition(row, col);
    frame.draw_rectangle(pos.getX(), pos.getY(), cellSize, cellSize, color, thickness);
}

void BoardCanvas::drawSelectionHighlight(int row, int col) {
    drawCellOutline(row, col, SELECTION_COLOR, SELECTION_THICKNESS);
}

void BoardCanvas::drawJumpHighlight(int row, int col) {
    drawCellOutline(row, col, JUMP_COLOR, JUMP_THICKNESS);
}

void BoardCanvas::drawRestProgress(int row, int col, double progress) {
    double clamped = std::min(1.0, std::max(0.0, progress));
    double remaining = 1.0 - clamped;  // bar shrinks as the rest counts down

    PixelPosition pos = getCellPosition(row, col);
    int barWidth = static_cast<int>(cellSize * remaining);
    int barY = pos.getY() + cellSize - REST_BAR_HEIGHT;

    frame.draw_rectangle(pos.getX(), barY, barWidth, REST_BAR_HEIGHT, REST_BAR_COLOR, cv::FILLED);
}

void BoardCanvas::show() { frame.show(); }

const std::string& BoardCanvas::getWindowName() const { return Img::windowName(); }

void BoardCanvas::drawText(std::string text){
    frame.put_text(text, 150, 300, 3.0,{255,255,255,0},10);
    frame.show();
}