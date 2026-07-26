#ifndef MOTION_VIEW_H
#define MOTION_VIEW_H

#include <nlohmann/json.hpp>

#include "PositionView.h"

#include "../../server/src/game/model/Motion.h"

class MotionView
{
private:
    bool active;
    PositionView from;
    PositionView to;
    long long startTime;
    long long endTime;

public:
    MotionView();

    MotionView(const Motion& motion);

    // All-fields constructor, used to reconstruct a MotionView from JSON on
    // a side (the client) that has no domain Motion object to convert from.
    MotionView(bool active,
               const PositionView& from,
               const PositionView& to,
               long long startTime,
               long long endTime);

    bool isActive() const;

    const PositionView& getFrom() const;
    const PositionView& getTo() const;

    long long getStartTime() const;
    long long getEndTime() const;

    double getProgress(long long currentTime) const;

    nlohmann::json toJson() const;
    static MotionView fromJson(const nlohmann::json& j);
};

#endif
