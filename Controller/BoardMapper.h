#ifndef BOARDMAPPER_H
#define BOARDMAPPER_H

#include "../model/Position.h"

class BoardMapper
{
public:
    static constexpr int CELL_SIZE = 100;

    Position pixelToCell(int x, int y) const;
};

#endif