#ifndef GAMESTATE_H
#define GAMESTATE_H

#include "Board.h"
#include <string>

class GameState
{
private:
    Board board;

    int selectedRow = -1;
    int selectedCol = -1;

    long long gameClock = 0;

    bool hasPendingMove = false;

    int fromRow = -1;
    int fromCol = -1;

    int toRow = -1;
    int toCol = -1;

public:
    GameState(const Board& b);

    void handleClick(int x, int y);

    void wait(int ms);

    void printBoard() const;
};

#endif