#ifndef IMOVEMENTRULE_H
#define IMOVEMENTRULE_H

#include <set>

#include "../Model/Board.h"
#include "../Model/Piece.h"
#include "../Model/Position.h"

class IMovementRule
{
public:
    virtual ~IMovementRule() = default;

    virtual std::set<Position> legalDestinations(
        const Board& board,
        const Piece& piece) const = 0;
};

#endif