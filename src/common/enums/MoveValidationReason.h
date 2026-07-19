#ifndef MOVEVALIDATIONREASON_H
#define MOVEVALIDATIONREASON_H

enum class MoveValidationReason
{
    Valid,
    SourceEmpty,
    DestinationOutsideBoard,
    FriendlyPieceAtDestination,
    IllegalMovement,
    PathBlocked,
    GameOver,
    MoveAlreadyInProgress,
    PieceResting
};

#endif