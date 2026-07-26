#include "BoardMapper.h"

PositionView BoardMapper::pixelToCell(int x, int y) const
{
    return PositionView(
        y / CELL_SIZE,
        x / CELL_SIZE);
}

PositionView BoardMapper::pixelToCell(const PixelPosition& pixel) const
{
    return pixelToCell(pixel.getX(), pixel.getY());
}