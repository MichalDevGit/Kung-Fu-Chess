#include "JumpView.h"
#include "../TimeProgress.h"

JumpView::JumpView()
    : active(false),
      position(0, 0),
      startTime(0),
      endTime(0)
{
}

bool JumpView::isActive() const
{
    return active;
}

const PositionView& JumpView::getPosition() const
{
    return position;
}

long long JumpView::getStartTime() const
{
    return startTime;
}

long long JumpView::getEndTime() const
{
    return endTime;
}

JumpView::JumpView(bool active,
                    const PositionView& position,
                    long long startTime,
                    long long endTime)
    : active(active),
      position(position),
      startTime(startTime),
      endTime(endTime)
{
}

double JumpView::getProgress(long long currentTime) const
{
    return computeProgress(startTime, endTime, currentTime);
}

nlohmann::json JumpView::toJson() const
{
    return nlohmann::json{
        {"active", active},
        {"position", position.toJson()},
        {"startTime", startTime},
        {"endTime", endTime}};
}

JumpView JumpView::fromJson(const nlohmann::json& j)
{
    return JumpView(
        j.at("active").get<bool>(),
        PositionView::fromJson(j.at("position")),
        j.at("startTime").get<long long>(),
        j.at("endTime").get<long long>());
}
