#include "PieceView.h"

// See PositionViewFromDomain.cpp for why this converting constructor lives in
// its own translation unit, separate from PieceView.cpp: it's the only
// PieceView code referencing the server-side domain Piece type.
PieceView::PieceView(const Piece& piece)
    : id(piece.getId()),
      type(piece.getType()),
      color(piece.getColor()),
      state(piece.getState()),
      position(piece.getPosition())
{
}
