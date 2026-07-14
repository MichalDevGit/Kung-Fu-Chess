#ifndef GAMEENGINE_H
#define GAMEENGINE_H

#include "../model/GameState.h"
#include "../rules/RuleEngine.h"
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

    void settleCompletedMotions();

    void settleCompletedJumps();

    int calculatePathLength(
        const Piece& piece,
        const Position& from,
        const Position& to) const;
        
public:
    GameEngine(const GameState& gameState);

    void advanceTime(long long milliseconds);

    MoveValidation requestMove(
        const Position& from,
        const Position& to);

    MoveValidation requestJump(
        const Position& position);

    void executeMove(const Motion& motion);
        
    const GameState& getGameState() const;

    bool hasPieceAt(const Position& position) const;

    bool hasActiveMotion() const;

    long long getCurrentTime() const;
};

#endif