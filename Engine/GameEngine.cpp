#include "GameEngine.h"

GameEngine::GameEngine(const GameState& gameState)
    : gameState(gameState)
{
}

MoveValidation GameEngine::requestMove(
    const Position& from,
    const Position& to)
{
    MoveValidation validation =
        ruleEngine.validateMove(...);

    if (!validation.isValid)
    {
        return validation;
    }

    executeMove(from, to);

    return validation;
}

const GameState& GameEngine::getGameState() const
{
    return gameState;
}

bool GameEngine::hasPieceAt(const Position& position) const
{
    return gameState.getBoard().containsPiece(position);
}

void GameEngine::executeMove(
    const Position& from,
    const Position& to)
{
    gameState.getBoard().movePiece(from, to);
}