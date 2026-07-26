#include "Motion.h"

Motion::Motion()
    : active(false),
      from(0,0),
      to(0,0),
      startTime(0),
      endTime(0)
{
}

Motion::Motion(
    const Position& from,
    const Position& to,
    long long startTime,
    long long endTime)

    : active(true),
      from(from),
      to(to),
      startTime(startTime),
      endTime(endTime)
{
}

const Position& Motion::getFrom() const
{
    return from;
}

const Position& Motion::getTo() const
{
    return to;
}

long long Motion::getStartTime() const
{
    return startTime;
}

long long Motion::getEndTime() const
{
    return endTime;
}

bool Motion::isFinished(long long currentTime) const
{
    return currentTime >= endTime;
}

bool Motion::hasStarted(long long currentTime) const
{
    return currentTime >= startTime;
}

bool Motion::isActive() const
{
    return active;
}


void Motion::activate()
{
    active = true;
}


void Motion::deactivate()
{
    active = false;
}