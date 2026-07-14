#ifndef JUMP_H
#define JUMP_H

#include "Position.h"

class Jump
{
private:
    bool active;
    Position position;
    long long startTime;
    long long endTime;

public:
    Jump();

    Jump(
        const Position& position,
        long long startTime,
        long long endTime);

    const Position& getPosition() const;

    long long getStartTime() const;

    long long getEndTime() const;

    bool isActive() const;

    void activate();

    void deactivate();

    bool isFinished(long long currentTime) const;
};

#endif