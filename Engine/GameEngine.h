#ifndef GAMEENGINE_H
#define GAMEENGINE_H

#include "../Model/GameState.h"
#include "../Rules/RuleEngine.h"
#include "../common/DTO/MoveValidation.h"

class GameEngine
{
private:
    GameState gameState;
    RuleEngine ruleEngine;
    Board& getBoard();
    const Board& getBoard() const;

public:
    GameEngine(const GameState& gameState);

    MoveValidation move(
        const Position& from,
        const Position& to);

    const GameState& getGameState() const;
};

#endif