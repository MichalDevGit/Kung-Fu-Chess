#ifndef BOARDMAPPER_H
#define BOARDMAPPER_H

#include "../model/Position.h"
#include "../../../common/PixelPosition.h"
#include "../../../common/Config/BoardConfig.h"

class BoardMapper
{
public:
    static constexpr int CELL_SIZE = BoardConfig::CELL_SIZE;

    Position pixelToCell(int x, int y) const;
    Position pixelToCell(const PixelPosition& pixel) const;
};

#endif