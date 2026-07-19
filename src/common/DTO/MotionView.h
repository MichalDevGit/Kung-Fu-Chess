#ifndef MOTION_VIEW_H
#define MOTION_VIEW_H

#include "PositionView.h"

#include "../../logic/model/Motion.h"

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

    bool isActive() const;

    const PositionView& getFrom() const;
    const PositionView& getTo() const;

    long long getStartTime() const;
    long long getEndTime() const;

    double getProgress(long long currentTime) const;
};

#endif
