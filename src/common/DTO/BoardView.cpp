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

PieceView BoardView::getPiece(int row, int col) const
{
    int index = row * cols + col;

    if (index >= 0 && index < static_cast<int>(pieces.size()))
    {
        return pieces[index];
    }

    return PieceView(); 
}