#include "GameState.h"
#include <string>

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