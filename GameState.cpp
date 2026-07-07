#include "GameState.h"
#include <string>
#include <cmath>

GameState::GameState(const Board& board)
    : board(board)
{
}

void GameState::handleClick(int x, int y)
{
    if (hasPendingMove)
        return;

    int row = y / 100;
    int col = x / 100;

    if (!board.isValidPosition(row, col))
        return;

    std::string piece = board.getPiece(row, col);

    if (selectedRow == -1)
    {
        if (piece != ".")
        {
            selectedRow = row;
            selectedCol = col;
        }

        return;
    }

    std::string selectedPiece = board.getPiece(selectedRow, selectedCol);

    if (piece != "." &&
        piece[0] == selectedPiece[0])
    {
        selectedRow = row;
        selectedCol = col;
    }
    else
    {
        if (!isLegalMove(selectedPiece,
            selectedRow,
            selectedCol,
            row,
            col))
        {
            return;
        }
        
        hasPendingMove = true;
        fromRow = selectedRow;
        fromCol = selectedCol;
        toRow = row;
        toCol = col;

        selectedRow = -1;
        selectedCol = -1;
    }
}

void GameState::wait(int ms)
{
    gameClock += ms;

    if (!hasPendingMove)
        return;

    std::string piece = board.getPiece(fromRow, fromCol);

    if (piece != ".")
    {
        board.setPiece(toRow, toCol, piece);
        board.setPiece(fromRow, fromCol, ".");
    }

    hasPendingMove = false;

    selectedRow = -1;
    selectedCol = -1;
    fromRow = -1;
    fromCol = -1;
    toRow = -1;
    toCol = -1;
}

void GameState::printBoard() const
{
    board.print();
}


bool GameState::isLegalMove(const std::string& piece,
    int fromRow,
    int fromCol,
    int toRow,
    int toCol) const
{
    int dr = std::abs(toRow - fromRow);
    int dc = std::abs(toCol - fromCol);

    switch (piece[1])
    {
        case 'K':
        return dr <= 1 && dc <= 1 && (dr != 0 || dc != 0);

        case 'R':
        return (dr == 0 && dc > 0) ||
        (dc == 0 && dr > 0);

        case 'B':
        return dr == dc && dr > 0;

        case 'Q':
        return (dr == dc && dr > 0) ||
        (dr == 0 && dc > 0) ||
        (dc == 0 && dr > 0);

        case 'N':
        return (dr == 2 && dc == 1) ||
        (dr == 1 && dc == 2);

        default:
        return false;
    }
}