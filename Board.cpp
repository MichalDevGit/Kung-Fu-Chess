#include "Board.h"
#include <iostream>


bool Board::isValidPosition(int row, int col) const
{
    return row >= 0 &&
           row < rows &&
           col >= 0 &&
           col < cols;
}

std::string Board::getPiece(int row, int col) const
{
    if (!isValidPosition(row, col))
        return "";

    return grid[row][col];
}

void Board::setPiece(int row, int col,const std::string& piece) const
{
    grid[row][col] = piece;
}

void Board::print() const
{
    for (int i = 0; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            std::cout << grid[i][j];

            if (j + 1 < cols)
                std::cout << " ";
        }

        std::cout << std::endl;
    }
}

int Board::getRows() const
{
    return rows;
}

int Board::getCols() const
{
    return cols;
}