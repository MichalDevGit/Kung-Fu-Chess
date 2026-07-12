#include "RookRule.h"

#include <cmath>

MoveValidation RookRule::isLegalMove(
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
        return { false, MoveValidationReason::IllegalMovement };
    }

    // צריח נע רק אופקית או אנכית
    if (dr != 0 && dc != 0)
    {
        return { false, MoveValidationReason::IllegalMovement };
    }

    // הנתיב חייב להיות פנוי
    if (!isPathClear(board, from, to))
    {
        return { false, MoveValidationReason::PathBlocked };
    }

    return { true, MoveValidationReason::Valid };
}

bool RookRule::isPathClear(
    const Board& board,
    const Position& from,
    const Position& to) const
{
    int rowStep = 0;
    int colStep = 0;

    if (to.getRow() > from.getRow())
        rowStep = 1;
    else if (to.getRow() < from.getRow())
        rowStep = -1;

    if (to.getCol() > from.getCol())
        colStep = 1;
    else if (to.getCol() < from.getCol())
        colStep = -1;

    Position current(
        from.getRow() + rowStep,
        from.getCol() + colStep);

    while (current != to)
    {
        if (board.containsPiece(current))
        {
            return false;
        }

        current.setPosition(
            current.getRow() + rowStep,
            current.getCol() + colStep);
    }

    return true;
}