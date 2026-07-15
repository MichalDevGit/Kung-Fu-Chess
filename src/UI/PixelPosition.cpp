#include "PixelPosition.h"

PixelPosition::PixelPosition()
    : x(0),
      y(0)
{
}

PixelPosition::PixelPosition(int x, int y)
    : x(x),
      y(y)
{
}

int PixelPosition::getX() const
{
    return x;
}

int PixelPosition::getY() const
{
    return y;
}

void PixelPosition::setX(int x)
{
    this->x = x;
}

void PixelPosition::setY(int y)
{
    this->y = y;
}

void PixelPosition::setPosition(int x, int y)
{
    this->x = x;
    this->y = y;
}

bool PixelPosition::operator==(const PixelPosition& other) const
{
    return x == other.x &&
           y == other.y;
}

bool PixelPosition::operator!=(const PixelPosition& other) const
{
    return !(*this == other);
}