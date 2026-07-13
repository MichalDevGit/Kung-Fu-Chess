#ifndef MOTION_H
#define MOTION_H

#include "Position.h"

class Motion
{
private:
    Position from;
    Position to;

    long long startTime;
    long long endTime;
public:
    Motion();
    Motion(
        const Position& from,
        const Position& to);

    const Position& getFrom() const;
    const Position& getTo() const;
    const Position& getStartTime() const;
    const Position& getEndTime() const;
    bool isFinished(long long currentTime) const;
    bool hasStarted(long long currentTime) const;

};

#endif