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
        ruleEngine.validateMove(getBoard(),from,to);

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
    return getBoard().containsPiece(position);
}

void GameEngine::executeMove(
    const Position& from,
    const Position& to)
{
    getBoard().movePiece(from, to);
}

Board& GameEngine::getBoard()
{
    return gameState.getBoard();
}

const Board& GameEngine::getBoard() const
{
    return gameState.getBoard();
}