#ifndef BOARDMAPPER_H
#define BOARDMAPPER_H

#include "../model/Position.h"
#include "../../common/PixelPosition.h"

class BoardMapper
{
public:
    static constexpr int CELL_SIZE = 100;

    Position pixelToCell(int x, int y) const;
    Position pixelToCell(const PixelPosition& pixel) const;
};

#endif