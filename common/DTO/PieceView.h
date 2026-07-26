#ifndef PIECE_VIEW_H
#define PIECE_VIEW_H

#include <nlohmann/json.hpp>

#include "PositionView.h"

#include "../enums/PieceColor.h"
#include "../enums/PieceType.h"
#include "../enums/PieceState.h"

#include "../../server/src/game/model/Position.h"
#include "../../server/src/game/model/Piece.h"

class PieceView
{
private:
    int id;

    PieceType type;
    PieceColor color;
    PieceState state;

    PositionView position;

public:
    PieceView();

    PieceView(int id,
              PieceType type,
              PieceColor color,
              PieceState state,
              const PositionView& position);

    PieceView(const Piece& piece);

    int getId() const;

    PieceType getType() const;
    PieceColor getColor() const;
    PieceState getState() const;

    bool isEmpty() const;
    std::string toString() const;

    const PositionView& getPosition() const;

    nlohmann::json toJson() const;
    static PieceView fromJson(const nlohmann::json& j);
};

#endif