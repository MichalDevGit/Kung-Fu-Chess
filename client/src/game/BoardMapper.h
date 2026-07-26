#ifndef BOARDMAPPER_H
#define BOARDMAPPER_H

#include "PixelPosition.h"
#include "../../../common/Config/BoardConfig.h"
#include "../../../common/DTO/PositionView.h"

// Pixel-to-cell translation is a rendering concern, not a domain-logic one --
// it returns the shared, transport-agnostic PositionView (a plain row/col
// pair) rather than the server-side domain Position type, so this class has
// no dependency on server/src/game/model at all.
class BoardMapper
{
public:
    static constexpr int CELL_SIZE = BoardConfig::CELL_SIZE;

    PositionView pixelToCell(int x, int y) const;
    PositionView pixelToCell(const PixelPosition& pixel) const;
};

#endif