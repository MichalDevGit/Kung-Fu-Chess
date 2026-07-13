#include "RealTimeArbiter.h"

RealTimeArbiter::RealTimeArbiter()
    : active(false)
{
}

bool RealTimeArbiter::hasActiveMotion() const
{
    return active;
}

void RealTimeArbiter::startMotion(const Motion& motion)
{
    currentMotion = motion;
    active = true;
}

void RealTimeArbiter::finishMotion()
{
    active = false;
}

const Motion& RealTimeArbiter::getCurrentMotion() const
{
    return currentMotion;
}