#include "GameState.h"

GameState::GameState(const Board& board)
    : board(board),
      gameOver(false),
      hasSelection(false),
      selectedPosition()
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

bool GameState::hasSelectedPiece() const
{
    return hasSelection;
}

Position GameState::getSelectedPosition() const
{
    return selectedPosition;
}

void GameState::setSelectedPosition(const Position& position)
{
    selectedPosition = position;
    hasSelection = true;
}

void GameState::clearSelection()
{
    hasSelection = false;
}