#include "PositionView.h"

PositionView::PositionView()
    : row(0),
      col(0)
{
}

PositionView::PositionView(int row,
                           int col)
    : row(row),
      col(col)
{
}

int PositionView::getRow() const
{
    return row;
}

int PositionView::getCol() const
{
    return col;
}

nlohmann::json PositionView::toJson() const
{
    return nlohmann::json{{"row", row}, {"col", col}};
}

PositionView PositionView::fromJson(const nlohmann::json& j)
{
    return PositionView(j.at("row").get<int>(), j.at("col").get<int>());
}

