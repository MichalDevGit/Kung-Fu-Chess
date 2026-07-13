#ifndef RULEENGINE_H
#define RULEENGINE_H

#include "../common/DTO/MoveValidation.h"
#include "../Model/Board.h"
#include "../Model/Position.h"
#include "RookRule.h"

class RuleEngine
{
public:
    MoveValidation validateMove(
        const Board& board,
        const Position& from,
        const Position& to) const;

private:
    RookRule rookRule;
};

#endif
