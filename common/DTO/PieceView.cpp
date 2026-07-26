#include "PieceView.h"

#include "../enums/EnumJson.h"

PieceView::PieceView()
    : id(-1),
      type(PieceType::Empty),
      color(PieceColor::None),
      state(PieceState::Idle),
      position()
{
}

PieceView::PieceView(int id,
                     PieceType type,
                     PieceColor color,
                     PieceState state,
                     const PositionView& position)
    : id(id),
      type(type),
      color(color),
      state(state),
      position(position)
{
}

int PieceView::getId() const
{
    return id;
}

PieceType PieceView::getType() const
{
    return type;
}

PieceColor PieceView::getColor() const
{
    return color;
}

PieceState PieceView::getState() const
{
    return state;
}

const PositionView& PieceView::getPosition() const
{
    return position;
}

bool PieceView::isEmpty() const
{
    return type == PieceType::Empty;
}

std::string PieceView::toString() const
{
    if (isEmpty())
        return ".";

    std::string result;

    result += (color == PieceColor::White) ? 'w' : 'b';

    switch (type)
    {
        case PieceType::King:
            result += 'K';
            break;

        case PieceType::Queen:
            result += 'Q';
            break;

        case PieceType::Rook:
            result += 'R';
            break;

        case PieceType::Bishop:
            result += 'B';
            break;

        case PieceType::Knight:
            result += 'N';
            break;

        case PieceType::Pawn:
            result += 'P';
            break;

        default:
            result = ".";
    }

    return result;
}

nlohmann::json PieceView::toJson() const
{
    return nlohmann::json{
        {"id", id},
        {"type", type},
        {"color", color},
        {"state", state},
        {"position", position.toJson()}};
}

PieceView PieceView::fromJson(const nlohmann::json& j)
{
    return PieceView(
        j.at("id").get<int>(),
        j.at("type").get<PieceType>(),
        j.at("color").get<PieceColor>(),
        j.at("state").get<PieceState>(),
        PositionView::fromJson(j.at("position")));
}
