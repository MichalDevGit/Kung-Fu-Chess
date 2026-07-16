#include "BoardMapper.h"

Position BoardMapper::pixelToCell(int x, int y) const
{
    return Position(
        y / CELL_SIZE,
        x / CELL_SIZE);
}

Position BoardMapper::pixelToCell(const PixelPosition& pixel) const
{
    return pixelToCell(pixel.getX(), pixel.getY());
}