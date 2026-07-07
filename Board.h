#ifndef BOARD_H
#define BOARD_H

#include <vector>
#include <string>

class Board {
private:
    std::vector<std::vector<std::string>> grid;
    int rows;
    int cols;

public:
    Board(const std::vector<std::vector<std::string>>& inputGrid) 
        : grid(inputGrid) {
        rows = grid.size();
        cols = (rows > 0) ? grid[0].size() : 0;
    }

    bool isValidPosition(int r, int c) const {
        return (r >= 0 && r < rows && c >= 0 && c < cols);
    }

    std::string getPiece(int r, int c) const {
        if (isValidPosition(r, c)) {
            return grid[r][c];
        }
        return ""; 
    }

    void setPiece(int r, int c, const std::string& piece) {
        if (isValidPosition(r, c)) {
            grid[r][c] = piece;
        }
    }
=
    int getRows() const { return rows; }
    int getCols() const { return cols; }
};

#endif