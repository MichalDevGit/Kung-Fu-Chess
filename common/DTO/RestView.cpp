#include "RestView.h"
#include "../TimeProgress.h"
#include "../enums/EnumJson.h"

RestView::RestView()
    : pieceId(-1),
      position(0, 0),
      startTime(0),
      endTime(0),
      kind(RestKind::Long)
{
}

RestView::RestView(int pieceId,
                   const PositionView& position,
                   long long startTime,
                   long long endTime,
                   RestKind kind)
    : pieceId(pieceId),
      position(position),
      startTime(startTime),
      endTime(endTime),
      kind(kind)
{
}

int RestView::getPieceId() const
{
    return pieceId;
}

const PositionView& RestView::getPosition() const
{
    return position;
}

long long RestView::getStartTime() const
{
    return startTime;
}

long long RestView::getEndTime() const
{
    return endTime;
}

RestKind RestView::getKind() const
{
    return kind;
}

double RestView::getProgress(long long currentTime) const
{
    return computeProgress(startTime, endTime, currentTime);
}

nlohmann::json RestView::toJson() const
{
    return nlohmann::json{
        {"pieceId", pieceId},
        {"position", position.toJson()},
        {"startTime", startTime},
        {"endTime", endTime},
        {"kind", kind}};
}

RestView RestView::fromJson(const nlohmann::json& j)
{
    return RestView(
        j.at("pieceId").get<int>(),
        PositionView::fromJson(j.at("position")),
        j.at("startTime").get<long long>(),
        j.at("endTime").get<long long>(),
        j.at("kind").get<RestKind>());
}
