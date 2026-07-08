#include "Piece.h"

Piece::Piece()
    : value(".")
{
}

Piece::Piece(const std::string& value)
    : value(value)
{
}

std::string Piece::toString() const
{
    return value;
}

bool Piece::isEmpty() const
{
    return value == ".";
}

char Piece::getColor() const
{
    if (isEmpty())
        return '\0';

    return value[0];
}

char Piece::getType() const
{
    if (isEmpty())
        return '\0';

    return value[1];
}

bool Piece::isWhite() const
{
    return getColor() == 'w';
}

bool Piece::isBlack() const
{
    return getColor() == 'b';
}

void Piece::setValue(const std::string& value)
{
    this->value = value;
}

bool Piece::operator==(const Piece& other) const
{
    return value == other.value;
}

bool Piece::operator!=(const Piece& other) const
{
    return !(*this == other);
}