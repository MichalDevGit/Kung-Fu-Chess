#include "RuleEngine.h"

MoveValidation RuleEngine::validateMove(
    const Board& board,
    const Position& from,
    const Position& to) const
{
    // מקור בתוך הלוח
    if (!board.isValidPosition(from))
    {
        return { false, MoveValidationReason::SourceOutsideBoard };
    }

    // יעד בתוך הלוח
    if (!board.isValidPosition(to))
    {
        return { false, MoveValidationReason::DestinationOutsideBoard };
    }

    // יש כלי במקור?
    const Piece* piece = board.getPiece(from);

    if (piece == nullptr)
    {
        return { false, MoveValidationReason::NoPieceAtSource };
    }

    // יש כלי מאותו צבע ביעד?
    const Piece* destination = board.getPiece(to);

    if (destination != nullptr &&
        destination->getColor() == piece->getColor())
    {
        return { false, MoveValidationReason::FriendlyPieceAtDestination };
    }

    switch (piece->getType())
    {
        case PieceType::Rook:
            return rookRule.isLegalMove(board, *piece, to);

        default:
            return { false, MoveValidationReason::UnsupportedPiece };
    }
}