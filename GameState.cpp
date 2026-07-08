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

    int row = y / 100;
    int col = x / 100;
    
    if (!board.isValidPosition(row, col))
    return;

    Piece piece = board.getPiece(row, col);

    if (isAirborne &&
        !piece.isEmpty() &&
        piece.getColor() != airbornePiece.getColor() &&
        row == airborneRow &&
        col == airborneCol)
        {
            board.setPiece(row, col, Piece("."));
            return;
        }
    
    if (hasPendingMove)
        return;

    if (selectedRow == -1)
    {
        if (!piece.isEmpty())
        {
            selectedRow = row;
            selectedCol = col;
        }
        
        return;
    }
    
    Piece selectedPiece = board.getPiece(selectedRow, selectedCol);

    if (!piece.isEmpty() &&
        piece.getColor() == selectedPiece.getColor())
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
        movingPiece = selectedPiece;
        selectedRow = -1;
        selectedCol = -1;
    }
}

int GameState::getMoveDistance(const Piece& piece,
    int fromRow,
    int fromCol,
    int toRow,
    int toCol) const
{
    int dr = std::abs(toRow - fromRow);
    int dc = std::abs(toCol - fromCol);

    switch (piece.getType())
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

    if (hasPendingMove && gameClock >= moveFinishTime)
    {
        Piece piece = movingPiece;
        Piece destination = board.getPiece(toRow, toCol);

        // אם יש כלי קופץ עדיין באוויר על משבצת היעד,
        // והוא אויב - הכלי המגיע נתפס.
        if (isAirborne &&
            toRow == airborneRow &&
            toCol == airborneCol &&
            piece.getColor() != airbornePiece.getColor())
        {
            board.setPiece(fromRow, fromCol, Piece("."));
        }
        else
        {
            board.setPiece(toRow, toCol, piece);
            board.setPiece(fromRow, fromCol, Piece("."));

            if (piece == Piece("wP") && toRow == 0)
            {
                board.setPiece(toRow, toCol, Piece("wQ"));
            }

            if (piece == Piece("bP") && toRow == board.getRows() - 1)
            {
                board.setPiece(toRow, toCol, Piece("bQ"));
            }

            if (destination == Piece("wK") || destination == Piece("bK"))
            {
                gameOver = true;
            }
        }

        hasPendingMove = false;
        moveFinishTime = 0;

        movingPiece = Piece();

        fromRow = -1;
        fromCol = -1;
        toRow = -1;
        toCol = -1;

        selectedRow = -1;
        selectedCol = -1;
    }

        // סיום קפיצה
    if (hasPendingJump &&
        gameClock >= jumpFinishTime)
    {
        hasPendingJump = false;
        jumpFinishTime = 0;

        isAirborne = false;
        airborneRow = -1;
        airborneCol = -1;
        airbornePiece = Piece();
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
            if (!board.getPiece(row, col).isEmpty())
                return false;

            row += rowStep;
            col += colStep;
        }

        return true;
}

bool GameState::isPawnMove(const Piece& piece,
    int fromRow,
    int fromCol,
    int toRow,
    int toCol) const
{
    Piece destination = board.getPiece(toRow, toCol);

    if (piece.getColor() == 'w')
    {
        // move forward one cell
        if (toCol == fromCol &&
            toRow == fromRow - 1 &&
            destination.isEmpty())
        {
            return true;
        }

        // move forward two cells from start row
        if (fromRow == board.getRows() - 1 &&
            toCol == fromCol &&
            toRow == fromRow - 2 &&
            board.getPiece(fromRow - 1, fromCol).isEmpty() &&
            destination.isEmpty())
        {
            return true;
        }

        // capture
        if (toRow == fromRow - 1 &&
            std::abs(toCol - fromCol) == 1 &&
            !destination.isEmpty() &&
            destination.getColor() == 'b')
        {
            return true;
        }
    }
    else
    {
        // move forward one cell
        if (toCol == fromCol &&
            toRow == fromRow + 1 &&
            destination.isEmpty())
        {
            return true;
        }

        // move forward two cells from start row
        if (fromRow == 0 &&
            toCol == fromCol &&
            toRow == fromRow + 2 &&
            board.getPiece(fromRow + 1, fromCol).isEmpty() &&
            destination.isEmpty())
        {
            return true;
        }

        // capture
        if (toRow == fromRow + 1 &&
            std::abs(toCol - fromCol) == 1 &&
            !destination.isEmpty() &&
            destination.getColor() == 'w')
        {
            return true;
        }
    }

    return false;
}

bool GameState::isLegalMove(const Piece& piece,
    int fromRow,
    int fromCol,
    int toRow,
    int toCol) const 
{

    Piece destination = board.getPiece(toRow, toCol);

    if (!destination.isEmpty() &&
        destination.getColor()== piece.getColor())
    {
        return false;
    }

    int dr = std::abs(toRow - fromRow);
    int dc = std::abs(toCol - fromCol);

    switch (piece.getType())
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

void GameState::jump(int x, int y)
{
    if (gameOver)
        return;

    if (hasPendingMove)
        return;

    int row = y / 100;
    int col = x / 100;

    if (!board.isValidPosition(row, col))
        return;

    if (board.getPiece(row, col).isEmpty())
        return;

    isAirborne = true;

    airborneRow = row;
    airborneCol = col;
    airbornePiece = board.getPiece(row, col);

    hasPendingJump = true;
    jumpFinishTime = gameClock + MOVE_DURATION;

    selectedRow = -1;
    selectedCol = -1;
}