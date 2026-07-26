#include "RookRule.h"

std::set<Position> RookRule::legalDestinations(
    const Board& board,
    const Piece& piece) const
{
    std::set<Position> destinations;

    scanDirection(board, piece, -1,  0, destinations); // למעלה
    scanDirection(board, piece,  1,  0, destinations); // למטה
    scanDirection(board, piece,  0, -1, destinations); // שמאלה
    scanDirection(board, piece,  0,  1, destinations); // ימינה

    return destinations;
}

void RookRule::scanDirection(
    const Board& board,
    const Piece& piece,
    int rowStep,
    int colStep,
    std::set<Position>& destinations) const
{
    Position current = piece.getPosition();

    current.setPosition(
        current.getRow() + rowStep,
        current.getCol() + colStep);

    while (board.isValidPosition(current))
    {
        const Piece* target = board.getPiece(current);

        if (target == nullptr)
        {
            destinations.insert(current);
        }
        else
        {
            if (target->getColor() != piece.getColor())
            {
                destinations.insert(current);
            }

            break;
        }

        current.setPosition(
            current.getRow() + rowStep,
            current.getCol() + colStep);
    }
}