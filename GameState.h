#ifndef GAMESTATE_H
#define GAMESTATE_H

#include "Board.h"

class GameState {
private:
    Board board;
    int selectedRow;
    int selectedCol;
    long long gameClock;

public:
    GameState(const Board& b) : board(b), selectedRow(-1), selectedCol(-1), gameClock(0) {}

    void handleClick(int x, int y);

    void addTime(int ms);

    void printBoard() const;

    bool isValidToken(const std::string& token);
};

#endif