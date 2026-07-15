#ifndef PIECE_VIEW_H
#define PIECE_VIEW_H

#include "PositionView.h"

#include "../enums/PieceColor.h"
#include "../enums/PieceType.h"
#include "../enums/PieceState.h"

#include "../../logic/model/Position.h"
#include "../../logic/model/Piece.h"

class PieceView
{
private:
    int id;

    PieceType type;
    PieceColor color;
    PieceState state;

    PositionView position;

public:
    PieceView();

    PieceView(int id,
              PieceType type,
              PieceColor color,
              PieceState state,
              const PositionView& position);

    PieceView(const Piece& piece);

    int getId() const;

    PieceType getType() const;
    PieceColor getColor() const;
    PieceState getState() const;

    bool isEmpty() const;
    std::string toString() const;

    const PositionView& getPosition() const;
};

#endif