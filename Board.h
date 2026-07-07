#ifndef BOARD_H
#define BOARD_H

#include <vector>
#include <string>

class Board
{
private:
    std::vector<std::vector<std::string>> grid;

public:
    Board(const std::vector<std::vector<std::string>>& grid);

    int getRows() const;
    int getCols() const;

    bool isValidPosition(int row, int col) const;

    std::string getPiece(int row, int col) const;

    void setPiece(int row, int col, const std::string& piece);

    void print() const;
};

#endif