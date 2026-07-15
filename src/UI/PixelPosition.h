#ifndef PIXEL_POSITION_H
#define PIXEL_POSITION_H

class PixelPosition {
private:
    int x;
    int y;
public:
    PixelPosition();
    PixelPosition(int x, int y);
    int getX() const;
    int getY() const;
    void setX(int x);
    void setY(int y);
    void setPosition(int x, int y);
};
#endif