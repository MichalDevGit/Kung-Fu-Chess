#include "RealTimeArbiter.h"

RealTimeArbiter::RealTimeArbiter()
    : active(false),
      currentTime(0),
      jumpActive(false)
{
}

bool RealTimeArbiter::hasActiveMotion() const
{
    return active;
}

bool RealTimeArbiter::hasActiveJump() const
{
    return jumpActive;
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

void RealTimeArbiter::startJump(
    const Position& position,
    long long duration)
{
    Jump jump(
        position,
        currentTime,
        currentTime + duration);

    currentJump = jump;
    jumpActive = true;
}

void RealTimeArbiter::finishMotion()
{
    active = false;
    currentMotion = Motion();
}

void RealTimeArbiter::finishJump()
{
    jumpActive = false;
    currentJump = Jump();
}

const Motion& RealTimeArbiter::getCurrentMotion() const
{
    return currentMotion;
}

const Jump& RealTimeArbiter::getCurrentJump() const
{
    return currentJump;
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

bool RealTimeArbiter::shouldFinishCurrentJump() const
{
    return jumpActive &&
           currentJump.isFinished(currentTime);
}