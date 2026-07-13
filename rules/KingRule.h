#ifndef KINGRULE_H
#define KINGRULE_H

#include <set>

#include "IMovementRule.h"

class KingRule : public IMovementRule
{
public:
    std::set<Position> legalDestinations(
        const Board& board,
        const Piece& piece) const override;
};

#endif
