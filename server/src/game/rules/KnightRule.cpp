#include "KnightRule.h"

std::set<Position> KnightRule::legalDestinations(
    const Board& board,
    const Piece& piece) const
{
    static const int offsets[8][2] = {
        {-2, -1}, {-2,  1},
        {-1, -2}, {-1,  2},
        { 1, -2}, { 1,  2},
        { 2, -1}, { 2,  1}
    };

    std::set<Position> destinations;
    const Position from = piece.getPosition();

    for (const auto& offset : offsets)
    {
        Position candidate(
            from.getRow() + offset[0],
            from.getCol() + offset[1]);

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

    return destinations;
}
