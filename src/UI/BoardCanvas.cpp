#include "BoardCanvas.h"


BoardCanvas::BoardCanvas(const std::string& board_path,
                         int cell_size)
    :
    cell_size(cell_size)
{
    board_image.read(board_path);
}


PixelPosition BoardCanvas::get_cell_position(int row,
                                             int col) const
{
    return PixelPosition(
        col * cell_size,
        row * cell_size
    );
}


void BoardCanvas::draw_piece(Img& piece,
                             int row,
                             int col)
{
    PixelPosition position =
        get_cell_position(row, col);


    Img& mutable_piece = piece;


    mutable_piece.draw_on(
        board_image,
        position.getX(),
        position.getY()
    );
}


void BoardCanvas::show()
{
    board_image.show();
}