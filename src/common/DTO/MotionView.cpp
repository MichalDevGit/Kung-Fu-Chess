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

MotionView::MotionView(const Motion& motion)
    : active(motion.isActive()),
      from(motion.getFrom()),
      to(motion.getTo()),
      startTime(motion.getStartTime()),
      endTime(motion.getEndTime())
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

double MotionView::getProgress(long long currentTime) const
{
    return computeProgress(startTime, endTime, currentTime);
}
