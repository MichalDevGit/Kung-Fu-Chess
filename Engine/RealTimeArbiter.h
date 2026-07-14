#ifndef REALTIMEARBITER_H
#define REALTIMEARBITER_H

#include "../model/Motion.h"
#include "../model/Jump.h"

class RealTimeArbiter
{
private:

    Motion currentMotion;
    long long currentTime;

    Jump currentJump;

public:
    RealTimeArbiter();

    bool hasActiveMotion() const;

    void startMotion(
        const Position& from,
        const Position& to,
        long long duration);

    void finishMotion();

    const Motion& getCurrentMotion() const;

    void advanceTime(long long milliseconds);

    long long getCurrentTime() const;

    bool shouldFinishCurrentMotion() const;


    void startJump(
        const Position& position,
        long long duration);
    
    void finishJump();
    
    bool hasActiveJump() const;
    
    const Jump& getCurrentJump() const;
    
    bool shouldFinishCurrentJump() const;
};

#endif