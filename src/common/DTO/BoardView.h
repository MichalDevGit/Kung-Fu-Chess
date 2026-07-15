#ifndef BOARD_VIEW_H
#define BOARD_VIEW_H

#include <vector>

#include "PieceView.h"

#include "../../logic/model/Board.h"


class BoardView
{
private:
    int rows;
    int cols;

    std::vector<PieceView> pieces;

public:
    BoardView();

    BoardView(int rows,
              int cols,
              const std::vector<PieceView>& pieces);

    BoardView(const Board& board);

    int getRows() const;
    int getCols() const;

    const std::vector<PieceView>& getPieces() const;

    PieceView getPiece(int row, int col) const;
};

#endif