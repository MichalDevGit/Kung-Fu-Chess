#ifndef PIECE_H
#define PIECE_H

#include <string>

#include "Position.h"
#include "../common/enums/PieceColor.h"
#include "../common/enums/PieceType.h"
#include "../common/enums/PieceState.h"

class Piece
{
private:
    int id;

    PieceType type;
    PieceColor color;
    PieceState state;

    Position position;

public:
    Piece();

    Piece(int id,
          PieceType type,
          PieceColor color,
          const Position& position);

    int getId() const;

    PieceType getType() const;
    PieceColor getColor() const;
    PieceState getState() const;

    Position getPosition() const;

    void setType(PieceType type);
    void setColor(PieceColor color);
    void setState(PieceState state);
    void setPosition(const Position& position);

    bool isEmpty() const;

    bool isWhite() const;
    bool isBlack() const;

    std::string toString() const;

    bool operator==(const Piece& other) const;
    bool operator!=(const Piece& other) const;
};

#endif