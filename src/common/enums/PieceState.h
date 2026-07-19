#ifndef PIECESTATE_H
#define PIECESTATE_H

enum class PieceState
{
    Idle,
    Moving,
    Captured,
    Jump,
    LongRest,
    ShortRest
};

#endif