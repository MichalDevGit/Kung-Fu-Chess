#ifndef GAMEENGINE_H
#define GAMEENGINE_H

#include "../Model/GameState.h"
#include "../Rules/RuleEngine.h"
#include "../common/DTO/MoveValidation.h"
#include "RealTimeArbiter.h"
#include "../common/enums/PieceType.h"

class GameEngine
{
private:
    GameState gameState;
    RuleEngine ruleEngine;
    RealTimeArbiter arbiter;
    static constexpr long long MILLIS_PER_SQUARE = 1000;
    Board& getBoard();
    const Board& getBoard() const;

    void advanceTime(long long milliseconds);

    int calculatePathLength(
        const Piece& piece,
        const Position& from,
        const Position& to) const;

public:
    GameEngine(const GameState& gameState);

    MoveValidation requestMove(
        const Position& from,
        const Position& to);

    void executeMove(const Motion& motion);
        
    const GameState& getGameState() const;

    bool hasPieceAt(const Position& position) const;
};

#endif