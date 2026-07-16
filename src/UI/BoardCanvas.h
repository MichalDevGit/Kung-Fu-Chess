#pragma once
#include "img.h"
#include "../common/PixelPosition.h"

class BoardCanvas {
private:
    Img background;
    Img frame;
    int cellSize;

public:
    BoardCanvas(const std::string& boardPath, int cellSize);
    void beginFrame();
    void drawPiece(Img& piece, int row, int col);
    void drawText(std::string text);
    void show();
    const std::string& getWindowName() const;
    PixelPosition getCellPosition(int row, int col) const;
};