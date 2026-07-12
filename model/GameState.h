#ifndef GAMESTATE_H
#define GAMESTATE_H

#include "Board.h"
#include "Position.h"

class GameState
{
private:
    Board board;

    bool gameOver;
    bool hasSelection;

    Position selectedPosition;

public:
    GameState(const Board& board);

    Board& getBoard();
    const Board& getBoard() const;

    bool isGameOver() const;
    void setGameOver(bool gameOver);

    bool hasSelectedPiece() const;

    Position getSelectedPosition() const;
    void setSelectedPosition(const Position& position);

    void clearSelection();
};

#endif