#include "Motion.h"

Motion::Motion(){
    from = Position(0,0);
    to = Position(0,0);
}
Motion::Motion(
    const Position& from,
    const Position& to)
    : from(from),
      to(to)
{
}

const Position& Motion::getFrom() const
{
    return from;
}

const Position& Motion::getTo() const
{
    return to;
}