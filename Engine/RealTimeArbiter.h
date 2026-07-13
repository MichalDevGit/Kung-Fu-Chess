#ifndef REALTIMEARBITER_H
#define REALTIMEARBITER_H

#include "../Model/Motion.h"

class RealTimeArbiter
{
private:
    bool active;
    Motion currentMotion;
    long long currentTime;

public:
    RealTimeArbiter();

    bool hasActiveMotion() const;

    void startMotion(const Motion& motion);

    void finishMotion();

    const Motion& getCurrentMotion() const;

    const Motion& getCurrentMotion() const;

    void advanceTime(long long milliseconds);

    long long getCurrentTime() const;

    bool shouldFinishCurrentMotion() const;

};

#endif