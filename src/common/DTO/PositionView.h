#ifndef POSITION_VIEW_H
#define POSITION_VIEW_H

#include "../../logic/model/Position.h"

class PositionView
{
private:
    int row;
    int col;

public:
    PositionView();
    PositionView(int row, int col);
    PositionView(const Position& position);

    int getRow() const;
    int getCol() const;
};

#endif