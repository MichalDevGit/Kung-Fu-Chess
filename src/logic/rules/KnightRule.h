#ifndef KNIGHTRULE_H
#define KNIGHTRULE_H

#include <set>

#include "IMovementRule.h"

class KnightRule : public IMovementRule
{
public:
    std::set<Position> legalDestinations(
        const Board& board,
        const Piece& piece) const override;
};

#endif
