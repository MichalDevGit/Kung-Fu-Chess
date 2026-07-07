#include "Board.h"
#include <iostream>

Board::Board(const std::vector<std::vector<std::string>>& grid)
    : grid(grid)
{
}

int Board::getRows() const
{
    return grid.size();
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

std::string Board::getPiece(int row, int col) const
{
    if (!isValidPosition(row, col))
        return "";

    return grid[row][col];
}

void Board::setPiece(int row, int col, const std::string& piece)
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
            std::cout << grid[i][j];

            if (j + 1 < getCols())
                std::cout << " ";
        }

        std::cout << std::endl;
    }
}