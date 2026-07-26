#include "MotionView.h"
#include "../TimeProgress.h"

MotionView::MotionView()
    : active(false),
      from(0, 0),
      to(0, 0),
      startTime(0),
      endTime(0)
{
}

bool MotionView::isActive() const
{
    return active;
}

const PositionView& MotionView::getFrom() const
{
    return from;
}

const PositionView& MotionView::getTo() const
{
    return to;
}

long long MotionView::getStartTime() const
{
    return startTime;
}

long long MotionView::getEndTime() const
{
    return endTime;
}

MotionView::MotionView(bool active,
                        const PositionView& from,
                        const PositionView& to,
                        long long startTime,
                        long long endTime)
    : active(active),
      from(from),
      to(to),
      startTime(startTime),
      endTime(endTime)
{
}

double MotionView::getProgress(long long currentTime) const
{
    return computeProgress(startTime, endTime, currentTime);
}

nlohmann::json MotionView::toJson() const
{
    return nlohmann::json{
        {"active", active},
        {"from", from.toJson()},
        {"to", to.toJson()},
        {"startTime", startTime},
        {"endTime", endTime}};
}

MotionView MotionView::fromJson(const nlohmann::json& j)
{
    return MotionView(
        j.at("active").get<bool>(),
        PositionView::fromJson(j.at("from")),
        PositionView::fromJson(j.at("to")),
        j.at("startTime").get<long long>(),
        j.at("endTime").get<long long>());
}
