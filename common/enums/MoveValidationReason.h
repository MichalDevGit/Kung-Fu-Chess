enum class MoveValidationReason
{
    Valid,
    SourceEmpty,
    DestinationOutsideBoard,
    FriendlyPieceAtDestination,
    IllegalMovement,
    PathBlocked,
    GameOver,
    MoveAlreadyInProgress
};