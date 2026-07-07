#include "Board.h"
#include <iostream>

void Board::setPiece(int r, int c, const std::string& piece) {
    if (isValidPosition(r, c)) {
        grid[r][c] = piece;
    }
}

void Board::print() const {
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            std::cout << grid[i][j] << (j == cols - 1 ? "" : " ");
        }
        std::cout << std::endl;
    }
}

