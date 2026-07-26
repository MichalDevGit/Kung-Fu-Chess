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

nlohmann::json BoardView::toJson() const
{
    nlohmann::json piecesJson = nlohmann::json::array();

    for (const PieceView& piece : pieces)
    {
        piecesJson.push_back(piece.toJson());
    }

    return nlohmann::json{{"rows", rows}, {"cols", cols}, {"pieces", piecesJson}};
}

BoardView BoardView::fromJson(const nlohmann::json& j)
{
    std::vector<PieceView> pieces;

    for (const auto& pieceJson : j.at("pieces"))
    {
        pieces.push_back(PieceView::fromJson(pieceJson));
    }

    return BoardView(j.at("rows").get<int>(), j.at("cols").get<int>(), pieces);
}