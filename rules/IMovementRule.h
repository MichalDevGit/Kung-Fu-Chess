#ifndef IMOVEMENTRULE_H
#define IMOVEMENTRULE_H

#include "../Model/Board.h"
#include "../Model/Piece.h"
#include "../Model/Position.h"

class IMovementRule
{
public:
    virtual ~IMovementRule() = default;

    virtual bool isLegalMove(
        const Board& board,
        const Piece& piece,
        const Position& to) const = 0;
};

#endif