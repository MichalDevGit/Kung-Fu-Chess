#ifndef MOTION_H
#define MOTION_H

#include "Position.h"

class Motion
{
private:
    bool active;
    Position from;
    Position to;
    long long startTime;
    long long endTime;
public:
    Motion();
    Motion(
        const Position& from,
        const Position& to,
        long long startTime, 
        long long endTime
    );

    const Position& getFrom() const;
    const Position& getTo() const;
    long long getStartTime() const;
    long long getEndTime() const;
    bool isFinished(long long currentTime) const;
    bool hasStarted(long long currentTime) const;

};

#endif