#include "PositionView.h"

PositionView::PositionView()
    : row(0),
      col(0)
{
}

PositionView::PositionView(int row,
                           int col)
    : row(row),
      col(col)
{
}

PositionView::PositionView(const Position& position)
    : row(position.getRow()),
      col(position.getCol())
{
}

int PositionView::getRow() const
{
    return row;
}

int PositionView::getCol() const
{
    return col;
}

