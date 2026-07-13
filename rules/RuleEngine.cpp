#include "RuleEngine.h"

#include "../Model/Piece.h"
#include "../common/enums/PieceType.h"

RuleEngine::RuleEngine()
{
    rulesByType[PieceType::Bishop] = &bishopRule;
    rulesByType[PieceType::King] = &kingRule;
    rulesByType[PieceType::Knight] = &knightRule;
    rulesByType[PieceType::Pawn] = &pawnRule;
    rulesByType[PieceType::Queen] = &queenRule;
    rulesByType[PieceType::Rook] = &rookRule;
}

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

    auto ruleIt = rulesByType.find(piece->getType());
    if (ruleIt == rulesByType.end())
    {
        return MoveValidation(
            false,
            MoveValidationReason::IllegalMovement);
    }

    std::set<Position> legalMoves =
        ruleIt->second->legalDestinations(board, *piece);

    if (legalMoves.count(to) > 0)
    {
        return MoveValidation(
            true,
            MoveValidationReason::Valid);
    }

    return MoveValidation(
        false,
        MoveValidationReason::IllegalMovement);
}