#include "PawnRule.h"

#include "../common/enums/PieceColor.h"

std::set<Position> PawnRule::legalDestinations(
    const Board& board,
    const Piece& piece) const
{
    std::set<Position> destinations;

    const Position from = piece.getPosition();
    const bool isWhite = piece.getColor() == PieceColor::White;
    const int direction = isWhite ? -1 : 1;
    const int startRow = isWhite ? board.getRows() - 2 : 1;

    Position oneStepForward(from.getRow() + direction, from.getCol());
    if (board.isValidPosition(oneStepForward) &&
        board.getPiece(oneStepForward) == nullptr)
    {
        destinations.insert(oneStepForward);

        Position twoStepsForward(from.getRow() + (2 * direction), from.getCol());
        if (from.getRow() == startRow &&
            board.isValidPosition(twoStepsForward) &&
            board.getPiece(twoStepsForward) == nullptr)
        {
            destinations.insert(twoStepsForward);
        }
    }

    Position leftCapture(from.getRow() + direction, from.getCol() - 1);
    if (board.isValidPosition(leftCapture))
    {
        const Piece* target = board.getPiece(leftCapture);
        if (target != nullptr && target->getColor() != piece.getColor())
        {
            destinations.insert(leftCapture);
        }
    }

    Position rightCapture(from.getRow() + direction, from.getCol() + 1);
    if (board.isValidPosition(rightCapture))
    {
        const Piece* target = board.getPiece(rightCapture);
        if (target != nullptr && target->getColor() != piece.getColor())
        {
            destinations.insert(rightCapture);
        }
    }

    return destinations;
}
