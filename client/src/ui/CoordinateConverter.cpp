#include "CoordinateConverter.h"

void CoordinateConverter::toPixel(
    const PositionView& position,
    PixelPosition& pixelPosition) const
{
    int x = position.getCol() * CELL_SIZE;
    int y = position.getRow() * CELL_SIZE;

    pixelPosition.setPosition(x, y);
}