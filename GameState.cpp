#include "GameState.h"
#include <string>
#include <cmath>

GameState::GameState(const Board& board)
    : board(board)
{
}

void GameState::handleClick(int x, int y)
{
    if (gameOver)
        return;

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
        moveFinishTime =
        gameClock +
        getMoveDistance(selectedPiece,
                        selectedRow,
                        selectedCol,
                        row,
                        col) * MOVE_DURATION;

        fromRow = selectedRow;
        fromCol = selectedCol;
        toRow = row;
        toCol = col;

        selectedRow = -1;
        selectedCol = -1;
    }
}

int GameState::getMoveDistance(const std::string& piece,
    int fromRow,
    int fromCol,
    int toRow,
    int toCol) const
{
    int dr = std::abs(toRow - fromRow);
    int dc = std::abs(toCol - fromCol);

    switch (piece[1])
    {
        case 'R':
            return dr + dc;

        case 'B':
            return dr;

        case 'Q':
            if (dr == dc)
                return dr;

            return dr + dc;

        case 'K':
            return 1;

        case 'N':
            return 1;

        case 'P':
            return 1;

        default:
            return 1;
    }
}

void GameState::wait(int ms)
{
    gameClock += ms;

    if (!hasPendingMove)
        return;
    
    if (gameClock < moveFinishTime)
        return;
    
    std::string piece = board.getPiece(fromRow, fromCol);
    std::string destination = board.getPiece(toRow, toCol);

    if (piece != ".")
    {
        board.setPiece(toRow, toCol, piece);
        board.setPiece(fromRow, fromCol, ".");

        if (piece == "wP" && toRow == 0)
        {
            board.setPiece(toRow, toCol, "wQ");
        }
    
        if (piece == "bP" && toRow == board.getRows() - 1)
        {
            board.setPiece(toRow, toCol, "bQ");
        }
    }
    
    hasPendingMove = false;
    moveFinishTime = 0;
    
    selectedRow = -1;
    selectedCol = -1;
    fromRow = -1;
    fromCol = -1;
    toRow = -1;
    toCol = -1;

    if (destination == "wK" || destination == "bK")
    {
        gameOver = true;
    }

}

void GameState::printBoard() const
{
    board.print();
}

bool GameState::isPathClear(int fromRow,
    int fromCol,
    int toRow,
    int toCol) const
    {
        int rowStep = 0;
        int colStep = 0;
        
        if (toRow > fromRow)
            rowStep = 1;
        else if (toRow < fromRow)
            rowStep = -1;

        if (toCol > fromCol)
            colStep = 1;
        else if (toCol < fromCol)
            colStep = -1;

        int row = fromRow + rowStep;
        int col = fromCol + colStep;

        while (row != toRow || col != toCol)
        {
            if (board.getPiece(row, col) != ".")
            return false;

            row += rowStep;
            col += colStep;
        }

        return true;
}

bool GameState::isPawnMove(const std::string& piece,
    int fromRow,
    int fromCol,
    int toRow,
    int toCol) const
{
    std::string destination = board.getPiece(toRow, toCol);

    if (piece[0] == 'w')
    {
        // move forward one cell
        if (toCol == fromCol &&
            toRow == fromRow - 1 &&
            destination == ".")
        {
            return true;
        }

        // move forward two cells from start row
        if (fromRow == board.getRows() - 1 &&
            toCol == fromCol &&
            toRow == fromRow - 2 &&
            board.getPiece(fromRow - 1, fromCol) == "." &&
            destination == ".")
        {
            return true;
        }

        // capture
        if (toRow == fromRow - 1 &&
            std::abs(toCol - fromCol) == 1 &&
            destination != "." &&
            destination[0] == 'b')
        {
            return true;
        }
    }
    else
    {
        // move forward one cell
        if (toCol == fromCol &&
            toRow == fromRow + 1 &&
            destination == ".")
        {
            return true;
        }

        // move forward two cells from start row
        if (fromRow == 0 &&
            toCol == fromCol &&
            toRow == fromRow + 2 &&
            board.getPiece(fromRow + 1, fromCol) == "." &&
            destination == ".")
        {
            return true;
        }

        // capture
        if (toRow == fromRow + 1 &&
            std::abs(toCol - fromCol) == 1 &&
            destination != "." &&
            destination[0] == 'w')
        {
            return true;
        }
    }

    return false;
}

bool GameState::isLegalMove(const std::string& piece,
    int fromRow,
    int fromCol,
    int toRow,
    int toCol) const 
{

    std::string destination = board.getPiece(toRow, toCol);

    if (destination != "." &&
        destination[0] == piece[0])
    {
        return false;
    }

    int dr = std::abs(toRow - fromRow);
    int dc = std::abs(toCol - fromCol);

    switch (piece[1])
    {
        case 'K':
            return dr <= 1 && dc <= 1 && (dr != 0 || dc != 0);

        case 'R':
            return ((dr == 0 && dc > 0) ||
            (dc == 0 && dr > 0))
            && isPathClear(fromRow, fromCol,
                        toRow, toCol);

        case 'B':
            return dr == dc &&
            dr > 0 &&
            isPathClear(fromRow, fromCol,
                    toRow, toCol);

        case 'Q':
            return (
                (dr == dc && dr > 0) ||
                (dr == 0 && dc > 0) ||
                (dc == 0 && dr > 0)
                )
                &&
                isPathClear(fromRow, fromCol,
                            toRow, toCol);

        case 'N':
            return (dr == 2 && dc == 1) ||
            (dr == 1 && dc == 2);

        case 'P':
            return isPawnMove(piece,
                fromRow,
                fromCol,
                toRow,
                toCol);

        default:
        return false;
    }
}