#ifndef COORDINATE_CONVERTER_H
#define COORDINATE_CONVERTER_H

#include "../common/DTO/PositionView.h"
#include "../common/PixelPosition.h"

class CoordinateConverter
{
private:
    static constexpr int CELL_SIZE = 100;

public:
    CoordinateConverter() = default;

    void toPixel(const PositionView& position,
        PixelPosition& pixelPosition) const;};

#endif