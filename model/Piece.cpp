#include "Piece.h"

Piece::Piece()
    : id(-1),
      type(PieceType::Empty),
      color(PieceColor::None),
      state(PieceState::Idle),
      position()
{
}

Piece::Piece(int id,
             PieceType type,
             PieceColor color,
             const Position& position)
    : id(id),
      type(type),
      color(color),
      state(PieceState::Idle),
      position(position)
{
}

int Piece::getId() const
{
    return id;
}

PieceType Piece::getType() const
{
    return type;
}

PieceColor Piece::getColor() const
{
    return color;
}

PieceState Piece::getState() const
{
    return state;
}

Position Piece::getPosition() const
{
    return position;
}

void Piece::setType(PieceType type)
{
    this->type = type;
}

void Piece::setColor(PieceColor color)
{
    this->color = color;
}

void Piece::setState(PieceState state)
{
    this->state = state;
}

void Piece::setPosition(const Position& position)
{
    this->position = position;
}

bool Piece::isEmpty() const
{
    return type == PieceType::Empty;
}

bool Piece::isWhite() const
{
    return color == PieceColor::White;
}

bool Piece::isBlack() const
{
    return color == PieceColor::Black;
}

std::string Piece::toString() const
{
    if (isEmpty())
        return ".";

    std::string result;

    result += (color == PieceColor::White) ? 'w' : 'b';

    switch (type)
    {
        case PieceType::King:
            result += 'K';
            break;

        case PieceType::Queen:
            result += 'Q';
            break;

        case PieceType::Rook:
            result += 'R';
            break;

        case PieceType::Bishop:
            result += 'B';
            break;

        case PieceType::Knight:
            result += 'N';
            break;

        case PieceType::Pawn:
            result += 'P';
            break;

        default:
            result = ".";
    }

    return result;
}

bool Piece::operator==(const Piece& other) const
{
    return id == other.id;
}

bool Piece::operator!=(const Piece& other) const
{
    return !(*this == other);
}