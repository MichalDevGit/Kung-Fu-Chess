#include "Board.h"
#include <stdexcept>

Board::Board(int rows, int cols)
    : rows(rows),
      cols(cols)
{
}

int Board::getRows() const
{
    return rows;
}

int Board::getCols() const
{
    return cols;
}

bool Board::isValidPosition(const Position& position) const
{
    return position.getRow() >= 0 &&
           position.getRow() < rows &&
           position.getCol() >= 0 &&
           position.getCol() < cols;
}

Piece* Board::getPiece(const Position& position)
{
    for (Piece& piece : pieces)
    {
        if (piece.getPosition() == position)
        {
            return &piece;
        }
    }

    return nullptr;
}

const Piece* Board::getPiece(const Position& position) const
{
    for (const Piece& piece : pieces)
    {
        if (piece.getPosition() == position)
        {
            return &piece;
        }
    }

    return nullptr;
}

void Board::addPiece(const Piece& piece)
{
    if (!isValidPosition(piece.getPosition()))
    {
        throw std::out_of_range("Piece position is outside the board");
    }

    if (containsPiece(piece.getPosition()))
    {
        throw std::invalid_argument("Cell is already occupied");
    }

    pieces.push_back(piece);
}

void Board::removePiece(const Position& position)
{
    for (auto it = pieces.begin(); it != pieces.end(); ++it)
    {
        if (it->getPosition() == position)
        {
            pieces.erase(it);
            return;
        }
    }
}

void Board::movePiece(const Position& from,
                      const Position& to)
{
    Piece* piece = getPiece(from);

    if (piece != nullptr)
    {
        piece->setPosition(to);
    }
}

const std::vector<Piece>& Board::getPieces() const
{
    return pieces;
}

bool Board::containsPiece(const Position& position) const{
    return getPiece(position) != nullptr;
}