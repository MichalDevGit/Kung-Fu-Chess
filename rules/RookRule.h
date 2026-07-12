#ifndef ROOKRULE_H
#define ROOKRULE_H

#include "IMovementRule.h"

class RookRule : public IMovementRule
{
public:
    MoveValidation isLegalMove(
        const Board& board,
        const Piece& piece,
        const Position& to) const override;

private:
    bool isPathClear(
        const Board& board,
        const Position& from,
        const Position& to) const;
};

#endif