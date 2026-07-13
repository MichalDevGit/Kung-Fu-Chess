#include "Motion.h"

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