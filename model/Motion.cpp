#include "Motion.h"

Motion::Motion(){
    from = Position(0,0);
    to = Position(0,0);
    startTime= 0;
    endTime=0;
}
Motion::Motion(
    const Position& from,
    const Position& to, long long startTime, long long endTime)
    : from(from),
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

const Position& Motion::getStartTime() const{
    return startTime;
}

const Position& Motion::getEndTime() const{
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