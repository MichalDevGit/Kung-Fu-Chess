#pragma once
#include "img.h"
#include "../common/PixelPosition.h"
#include "../common/DTO/PositionView.h"

class BoardCanvas {
private:
    Img background;
    Img frame;
    int cellSize;

    void drawCellOutline(int row, int col, const cv::Scalar& color, int thickness);

    static const cv::Scalar SELECTION_COLOR;
    static const cv::Scalar JUMP_COLOR;
    static const cv::Scalar REST_BAR_COLOR;
    static constexpr int SELECTION_THICKNESS = 4;
    static constexpr int JUMP_THICKNESS = 4;
    static constexpr int REST_BAR_HEIGHT = 10;

public:
    BoardCanvas(const std::string& boardPath, int cellSize);
    void beginFrame();
    void drawPiece(Img& piece, int row, int col);
    void drawPieceAtPixel(Img& piece, const PixelPosition& pixelPos);
    void drawText(std::string text);
    void show();
    const std::string& getWindowName() const;
    PixelPosition getCellPosition(int row, int col) const;
    PixelPosition getInterpolatedPosition(const PositionView& from, const PositionView& to, double progress) const;
    void drawSelectionHighlight(int row, int col);
    void drawJumpHighlight(int row, int col);
    void drawRestProgress(int row, int col, double progress);
};