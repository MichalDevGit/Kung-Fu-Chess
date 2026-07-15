#include "PieceView.h"

PieceView::PieceView()
    : id(-1),
      type(PieceType::Empty),
      color(PieceColor::None),
      state(PieceState::Idle),
      position()
{
}

PieceView::PieceView(int id,
                     PieceType type,
                     PieceColor color,
                     PieceState state,
                     const PositionView& position)
    : id(id),
      type(type),
      color(color),
      state(state),
      position(position)
{
}

PieceView::PieceView(const Piece& piece)
    : id(piece.getId()),
      type(piece.getType()),
      color(piece.getColor()),
      state(piece.getState()),
      position(piece.getPosition())
{
}

int PieceView::getId() const
{
    return id;
}

PieceType PieceView::getType() const
{
    return type;
}

PieceColor PieceView::getColor() const
{
    return color;
}

PieceState PieceView::getState() const
{
    return state;
}

const PositionView& PieceView::getPosition() const
{
    return position;
}