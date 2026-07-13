#ifndef MOTION_H
#define MOTION_H

#include "Position.h"

class Motion
{
public:
    Motion(
        const Position& from,
        const Position& to);

    const Position& getFrom() const;
    const Position& getTo() const;

private:
    Position from;
    Position to;
};

#endif