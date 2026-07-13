#include "BoardMapper.h"

Position BoardMapper::pixelToCell(int x, int y) const
{
    return Position(
        y / CELL_SIZE,
        x / CELL_SIZE);
}