#include "Position.h"

Position::Position()
    : row(0),
      col(0)
{
}

Position::Position(int row, int col)
    : row(row),
      col(col)
{
}

int Position::getRow() const
{
    return row;
}

int Position::getCol() const
{
    return col;
}

void Position::setRow(int row)
{
    this->row = row;
}

void Position::setCol(int col)
{
    this->col = col;
}

void Position::setPosition(int row, int col)
{
    this->row = row;
    this->col = col;
}

bool Position::operator==(const Position& other) const
{
    return row == other.row &&
           col == other.col;
}

bool Position::operator!=(const Position& other) const
{
    return !(*this == other);
}
bool Position::operator<(const Position& other) const
{
    if (row != other.row)
    {
        return row < other.row;
    }

    return col < other.col;
}