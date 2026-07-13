#include "KingRule.h"

std::set<Position> KingRule::legalDestinations(
    const Board& board,
    const Piece& piece) const
{
    std::set<Position> destinations;
    const Position from = piece.getPosition();

    for (int rowStep = -1; rowStep <= 1; ++rowStep)
    {
        for (int colStep = -1; colStep <= 1; ++colStep)
        {
            if (rowStep == 0 && colStep == 0)
            {
                continue;
            }

            Position candidate(
                from.getRow() + rowStep,
                from.getCol() + colStep);

            if (!board.isValidPosition(candidate))
            {
                continue;
            }

            const Piece* target = board.getPiece(candidate);
            if (target == nullptr || target->getColor() != piece.getColor())
            {
                destinations.insert(candidate);
            }
        }
    }

    return destinations;
}
