#include "RookRule.h"
#include <cmath>

bool RookRule::isLegalMove(
    const Board& board,
    const Piece& piece,
    const Position& to) const
{
    Position from = piece.getPosition();

    int dr = std::abs(to.getRow() - from.getRow());
    int dc = std::abs(to.getCol() - from.getCol());

    // חייב לזוז
    if (dr == 0 && dc == 0)
    {
        return false;
    }

    // רק אופקי או אנכי
    if (dr != 0 && dc != 0)
    {
        return false;
    }

    // חסימות בדרך
    if (!isPathClear(board, from, to))
    {
        return false;
    }

    const Piece* destination = board.getPiece(to);

    // יש כלי מאותו צבע
    if (destination != nullptr &&
        destination->getColor() == piece.getColor())
    {
        return false;
    }

    return true;
}