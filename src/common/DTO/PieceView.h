#ifndef PIECE_VIEW_H
#define PIECE_VIEW_H

#include "PositionView.h"

#include "../../common/enums/PieceColor.h"
#include "../../common/enums/PieceType.h"
#include "../../common/enums/PieceState.h"

#include "../engine/Piece.h"

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

    const PositionView& getPosition() const;
};

#endif