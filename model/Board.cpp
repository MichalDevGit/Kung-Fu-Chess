#include "Board.h"

#include <iostream>

Board::Board(const std::vector<std::vector<Piece>>& grid)
    : grid(grid)
{
}

int Board::getRows() const
{
    return static_cast<int>(grid.size());
}

int Board::getCols() const
{
    if (grid.empty())
        return 0;

    return grid[0].size();
}

bool Board::isValidPosition(int row, int col) const
{
    return row >= 0 &&
           row < getRows() &&
           col >= 0 &&
           col < getCols();
}

Piece Board::getPiece(int row, int col) const
{
    if (!isValidPosition(row, col))
        return Piece();

    return grid[row][col];
}

void Board::setPiece(int row, int col, const Piece& piece)
{
    if (isValidPosition(row, col))
        grid[row][col] = piece;
}

void Board::print() const
{
    for (int i = 0; i < getRows(); i++)
    {
        for (int j = 0; j < getCols(); j++)
        {
            std::cout << grid[i][j].toString();

            if (j + 1 < getCols())
                std::cout << " ";
        }

        std::cout << std::endl;
    }
}