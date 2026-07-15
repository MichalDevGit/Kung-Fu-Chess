#include "BoardView.h"

BoardView::BoardView()
    : rows(0),
      cols(0)
{
}

BoardView::BoardView(int rows,
                     int cols,
                     const std::vector<PieceView>& pieces)
    : rows(rows),
      cols(cols),
      pieces(pieces)
{
}

BoardView::BoardView(const Board& board)
    : rows(board.getRows()),
      cols(board.getCols())
{
    for (const Piece& piece : board.getPieces())
    {
        pieces.emplace_back(piece);
    }
}

int BoardView::getRows() const
{
    return rows;
}

int BoardView::getCols() const
{
    return cols;
}

const std::vector<PieceView>& BoardView::getPieces() const
{
    return pieces;
}