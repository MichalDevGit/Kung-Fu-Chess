#ifndef PIECE_STATE_TO_STRING_H
#define PIECE_STATE_TO_STRING_H

#include "PieceState.h"
#include <string>

inline std::string pieceStateToString(PieceState state)
{
    switch (state)
    {
        case PieceState::Idle:     return "idle";
        case PieceState::Moving:   return "moving";
        case PieceState::Captured: return "captured";
        case PieceState::Jump:     return "jump";
        default:                   return "idle";
    }
}

#endif
