#include "RuleEngine.h"

#include "../Model/Piece.h"
#include "../common/enums/PieceType.h"

MoveValidation RuleEngine::validateMove(
    const Board& board,
    const Position& from,
    const Position& to) const
{
    const Piece* piece = board.getPiece(from);

    if (piece == nullptr)
    {
        return MoveValidation(
            false,
            MoveValidationReason::SourceEmpty);
    }

    switch (piece->getType())
    {
        case PieceType::Rook:
        {
            auto legalMoves =
                rookRule.legalDestinations(board, *piece);

            if (legalMoves.count(to) > 0)
            {
                return MoveValidation(
                    true,
                    MoveValidationReason::Valid);
            }

            break;
        }

        default:
            return MoveValidation(
                false,
                MoveValidationReason::IllegalMovement);
    }

    return MoveValidation(
        false,
        MoveValidationReason::IllegalMovement);
}