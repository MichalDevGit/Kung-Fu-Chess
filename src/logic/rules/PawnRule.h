#ifndef PAWNRULE_H
#define PAWNRULE_H

#include <set>
#include "../../common/enums/PieceColor.h"

#include "IMovementRule.h"

class PawnRule : public IMovementRule
{
public:
    std::set<Position> legalDestinations(
        const Board& board,
        const Piece& piece) const override;
};

#endif
