#pragma once
#include "img.h"
#include "../common/PixelPosition.h"

class BoardCanvas {
private:
    Img boardImage;
    int cellSize;

public:
    BoardCanvas(const std::string& boardPath, int cellSize);
    void drawPiece(Img& piece, int row, int col);
    void show();
    PixelPosition getCellPosition(int row, int col) const;
};