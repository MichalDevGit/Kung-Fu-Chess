#include "Jump.h"

Jump::Jump()
    : active(false),
      position(0, 0),
      startTime(0),
      endTime(0)
{
}

Jump::Jump(
    const Position& position,
    long long startTime,
    long long endTime)
    : active(true),
      position(position),
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

bool Jump::isActive() const
{
    return active;
}


void Jump::activate()
{
    active = true;
}


void Jump::deactivate()
{
    active = false;
}