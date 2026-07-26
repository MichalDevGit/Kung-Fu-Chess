#ifndef EVENTS_H
#define EVENTS_H

#include "../../logic/model/Position.h"
#include "../enums/PieceColor.h"
#include "../enums/PieceType.h"

// Event payloads published by GameEngine over the EventBus (see EventBus.h).
// Unlike EventBus.h itself, these depend on logic/model -- same layering as
// common/DTO, which also converts logic-layer types for consumers outside it.

// A new game session has started. Target: game-start animations.
struct GameStartedEvent
{
};

// A move finished settling onto the board. Target: move logs, movement sound.
struct MoveExecutedEvent
{
    int pieceId;
    Position from;
    Position to;
};

// A piece was removed from the board (ordinary capture or ambush capture).
// Target: capture sound effects, score updates.
struct PieceCapturedEvent
{
    int pieceId;
    PieceColor color;
    PieceType type;
    Position position;
};

// The game has ended (a king was captured). Target: score updates (final),
// end-of-game animation.
struct GameOverEvent
{
};

#endif
