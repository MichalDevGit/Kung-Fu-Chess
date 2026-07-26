#include "BoardView.h"

// See PositionViewFromDomain.cpp for why this converting constructor lives in
// its own translation unit, separate from BoardView.cpp: it's the only
// BoardView code referencing the server-side domain Board/Position types.
BoardView::BoardView(const Board& board)
    : rows(board.getRows()),
      cols(board.getCols())
{
    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < cols; ++col)
        {
            const Piece* piece = board.getPiece(Position(row, col));

            if (piece != nullptr)
            {
                pieces.emplace_back(*piece);
            }
            else
            {
                pieces.emplace_back(
                    PieceView(-1, PieceType::Empty, PieceColor::None,
                              PieceState::Idle, PositionView(row, col)));
            }
        }
    }
}
