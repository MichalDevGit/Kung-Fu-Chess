#include "Jump.h"

Jump::Jump()
    : position(0, 0),
      startTime(0),
      endTime(0)
{
}

Jump::Jump(
    const Position& position,
    long long startTime,
    long long endTime)
    : position(position),
      startTime(startTime),
      endTime(endTime)
{
}

const Position& Jump::getPosition() const
{
    return position;
}

long long Jump::getStartTime() const
{
    return startTime;
}

long long Jump::getEndTime() const
{
    return endTime;
}

bool Jump::isFinished(long long currentTime) const
{
    return currentTime >= endTime;
}