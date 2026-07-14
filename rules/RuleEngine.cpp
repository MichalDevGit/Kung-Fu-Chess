#include "RuleEngine.h"

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
    if (!board.isValidPosition(from) ||
        !board.isValidPosition(to))
    {
        return MoveValidation(
            false,
            MoveValidationReason::DestinationOutsideBoard);
    }

    const Piece* piece = board.getPiece(from);

    if (piece == nullptr)
    {
        return MoveValidation(
            false,
            MoveValidationReason::SourceEmpty);
    }

    const Piece* destinationPiece = board.getPiece(to);

    if (destinationPiece != nullptr &&
        destinationPiece->getColor() == piece->getColor())
    {
        return MoveValidation(
            false,
            MoveValidationReason::FriendlyPieceAtDestination);
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