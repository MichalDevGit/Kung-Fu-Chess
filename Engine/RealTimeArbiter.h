#ifndef REALTIMEARBITER_H
#define REALTIMEARBITER_H

#include "../Model/Motion.h"

class RealTimeArbiter
{
public:
    RealTimeArbiter();

    bool hasActiveMotion() const;

    void startMotion(const Motion& motion);

    void finishMotion();

    const Motion& getCurrentMotion() const;

private:
    bool active;
    Motion currentMotion;
};

#endif