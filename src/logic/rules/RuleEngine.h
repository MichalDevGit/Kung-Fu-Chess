#ifndef RULEENGINE_H
#define RULEENGINE_H

#include <map>

#include "../../common/DTO/MoveValidation.h"
#include "../../common/enums/PieceType.h"
#include "../model/Board.h"
#include "../model/Position.h"
#include "../model/Piece.h"
#include "BishopRule.h"
#include "KingRule.h"
#include "KnightRule.h"
#include "PawnRule.h"
#include "QueenRule.h"
#include "RookRule.h"

class RuleEngine
{
public:
    RuleEngine();

    MoveValidation validateMove(
        const Board& board,
        const Position& from,
        const Position& to) const;

private:
    BishopRule bishopRule;
    KingRule kingRule;
    KnightRule knightRule;
    PawnRule pawnRule;
    QueenRule queenRule;
    RookRule rookRule;
    std::map<PieceType, const IMovementRule*> rulesByType;
};

#endif
