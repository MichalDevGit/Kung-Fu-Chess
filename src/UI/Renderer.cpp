#include "Renderer.h"


Renderer::Renderer(BoardCanvas& canvas,
                   SpriteManager& sprite_manager)
    :
    canvas(canvas),
    sprite_manager(sprite_manager)
{
}


void Renderer::draw_board(const BoardView& snapshot)
{
    for (int row = 0; row < snapshot.getRows(); row++)
    {
        for (int col = 0; col < snapshot.getCols(); col++)
        {
            PieceView piece =
                snapshot.getPiece(row, col);


            if (piece.isEmpty())
                continue;


            Img sprite =
                sprite_manager.get_piece_sprite(
                    piece,
                    "idle",
                    1
                );


            canvas.draw_piece(
                sprite,
                row,
                col
            );
        }
    }
}


void Renderer::render(const BoardView& snapshot)
{
    draw_board(snapshot);

    canvas.show();
}