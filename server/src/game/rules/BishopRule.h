#ifndef BISHOPRULE_H
#define BISHOPRULE_H

#include <set>

#include "IMovementRule.h"

class BishopRule : public IMovementRule
{
public:
    std::set<Position> legalDestinations(
        const Board& board,
        const Piece& piece) const override;

private:
    void scanDirection(
        const Board& board,
        const Piece& piece,
        int rowStep,
        int colStep,
        std::set<Position>& destinations) const;
};

#endif
