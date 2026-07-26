#ifndef POSITION_VIEW_H
#define POSITION_VIEW_H

#include <nlohmann/json.hpp>

#include "../../server/src/game/model/Position.h"

class PositionView
{
private:
    int row;
    int col;

public:
    PositionView();
    PositionView(int row, int col);
    PositionView(const Position& position);

    int getRow() const;
    int getCol() const;

    nlohmann::json toJson() const;
    static PositionView fromJson(const nlohmann::json& j);
};

#endif