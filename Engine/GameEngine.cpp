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

    Motion motion(from, to);
    arbiter.startMotion(motion);

    executeMove(motion);

    arbiter.finishMotion();

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

void GameEngine::executeMove(const Motion& motion)
{
    getBoard().movePiece(
        motion.getFrom(),
        motion.getTo());
}

Board& GameEngine::getBoard()
{
    return gameState.getBoard();
}

const Board& GameEngine::getBoard() const
{
    return gameState.getBoard();
}