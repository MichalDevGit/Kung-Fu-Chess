#include "RealTimeArbiter.h"

RealTimeArbiter::RealTimeArbiter()
    : currentTime(0)
{
}

bool RealTimeArbiter::hasActiveMotion() const
{
    return currentMotion.isActive();
}

bool RealTimeArbiter::hasActiveJump() const
{
    return currentJump.isActive();
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
}

void RealTimeArbiter::finishMotion()
{
    currentMotion.deactivate();
}

void RealTimeArbiter::finishJump()
{
    currentJump.deactivate();
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

bool RealTimeArbiter::shouldFinishCurrentMotion() const
{
    return currentMotion.isActive() &&
           currentMotion.isFinished(currentTime);
}

bool RealTimeArbiter::shouldFinishCurrentJump() const
{
    return currentJump.isActive() &&
           currentJump.isFinished(currentTime);
}