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
        ruleEngine.validateMove(
            gameState.getBoard(),
            from,
            to);

    if (!validation.isValid)
    {
        return validation;
    }

    gameState.getBoard().movePiece(from, to);

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