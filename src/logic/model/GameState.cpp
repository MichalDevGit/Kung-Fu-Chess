#include "GameState.h"

GameState::GameState(const Board& board)
    : board(board),
      gameOver(false)
{
}

Board& GameState::getBoard()
{
    return board;
}

const Board& GameState::getBoard() const
{
    return board;
}

bool GameState::isGameOver() const
{
    return gameOver;
}

void GameState::setGameOver(bool gameOver)
{
    this->gameOver = gameOver;
}

