#ifndef COORDINATE_CONVERTER_H
#define COORDINATE_CONVERTER_H

#include "../../../common/DTO/PositionView.h"
#include "../game/PixelPosition.h"
#include "../../../common/Config/BoardConfig.h"

class CoordinateConverter
{
private:
    static constexpr int CELL_SIZE = BoardConfig::CELL_SIZE;

public:
    CoordinateConverter() = default;

    void toPixel(const PositionView& position,
        PixelPosition& pixelPosition) const;};

#endif