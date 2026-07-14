#include "RealTimeArbiter.h"

RealTimeArbiter::RealTimeArbiter()
    : active(false),
      currentTime(0)
{
}

bool RealTimeArbiter::hasActiveMotion() const
{
    return active;
}

void RealTimeArbiter::startMotion(
    const Position& from,
    const Position& to,
    long long duration
)
{
    Motion motion(
        from,
        to,
        currentTime,
        currentTime + duration);

    currentMotion = motion;
    active = true;
}

void RealTimeArbiter::finishMotion()
{
    active = false;
    currentMotion = Motion();
}

const Motion& RealTimeArbiter::getCurrentMotion() const
{
    return currentMotion;
}

void RealTimeArbiter::advanceTime(long long milliseconds)
{
    currentTime += milliseconds;
}

long long RealTimeArbiter::getCurrentTime() const
{
    return currentTime;
}

bool RealTimeArbiter::shouldFinishCurrentMotion() const{
    return active &&
    currentMotion.isFinished(currentTime);
}