#pragma once

#include <string>

#include "img.h"
#include "PixelPosition.h"


class BoardCanvas {
private:
    Img board_image;
    int cell_size;


public:
    BoardCanvas(const std::string& board_path,
                int cell_size);


    void draw_piece(Img& piece,
                    int row,
                    int col);


    void show();


    PixelPosition get_cell_position(int row,
                                    int col) const;
};