switch (piece->getType())
{
    case PieceType::Rook:
    {
        auto legalMoves =
            rookRule.legalDestinations(board, *piece);

        if (legalMoves.contains(to))
        {
            return MoveValidation(
                true,
                MoveValidationReason::None);
        }

        break;
    }

    default:
        return MoveValidation(
            false,
            MoveValidationReason::UnsupportedPiece);
}

return MoveValidation(
    false,
    MoveValidationReason::IllegalMove);