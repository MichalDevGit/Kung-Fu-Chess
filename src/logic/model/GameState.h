#ifndef GAMESTATE_H
#define GAMESTATE_H

#include "Board.h"
#include "Position.h"

class GameState
{
private:
    Board board;

    bool gameOver;

public:
    GameState(const Board& board);

    Board& getBoard();
    const Board& getBoard() const;

    bool isGameOver() const;
    void setGameOver(bool gameOver);


};

#endif