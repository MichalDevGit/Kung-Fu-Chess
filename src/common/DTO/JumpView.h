#ifndef JUMP_VIEW_H
#define JUMP_VIEW_H

#include "PositionView.h"

#include "../../logic/model/Jump.h"

class JumpView
{
private:
    bool active;
    PositionView position;
    long long startTime;
    long long endTime;

public:
    JumpView();

    JumpView(const Jump& jump);

    bool isActive() const;

    const PositionView& getPosition() const;

    long long getStartTime() const;
    long long getEndTime() const;

    double getProgress(long long currentTime) const;
};

#endif
