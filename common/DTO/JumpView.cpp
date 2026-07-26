#include "JumpView.h"
#include "../TimeProgress.h"

JumpView::JumpView()
    : active(false),
      position(0, 0),
      startTime(0),
      endTime(0)
{
}

JumpView::JumpView(const Jump& jump)
    : active(jump.isActive()),
      position(jump.getPosition()),
      startTime(jump.getStartTime()),
      endTime(jump.getEndTime())
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

double JumpView::getProgress(long long currentTime) const
{
    return computeProgress(startTime, endTime, currentTime);
}
