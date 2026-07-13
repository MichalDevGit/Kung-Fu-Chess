#ifndef RULEENGINE_H
#define RULEENGINE_H

#include "../Common/MoveValidation.h"
#include "../Model/Board.h"

#include "RookRule.h"

class RuleEngine
{
private:
    RookRule rookRule;

public:
    MoveValidation validateMove(
        const Board& board,
        const Position& from,
        const Position& to) const;
};

#endif