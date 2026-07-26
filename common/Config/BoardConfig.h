#ifndef COMMON_CONFIG_BOARD_CONFIG_H
#define COMMON_CONFIG_BOARD_CONFIG_H

// Single source of truth for the pixel size of one board square. Previously
// duplicated independently in BoardMapper, CoordinateConverter, and the
// BoardCanvas/SpriteManager construction calls in main.cpp.
namespace BoardConfig
{
    constexpr int CELL_SIZE = 100;
}

#endif
