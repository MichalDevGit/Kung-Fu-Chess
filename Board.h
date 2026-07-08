#ifndef BOARD_H
#define BOARD_H

#include <vector>
#include "Piece.h"

class Board
{
private:
    std::vector<std::vector<Piece>> grid;

public:
    Board(const std::vector<std::vector<Piece>>& grid);

    int getRows() const;
    int getCols() const;

    bool isValidPosition(int row, int col) const;

    Piece getPiece(int row, int col) const;

    void setPiece(int row, int col, const Piece& piece);

    void print() const;
};

#endif