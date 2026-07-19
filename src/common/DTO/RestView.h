#ifndef REST_VIEW_H
#define REST_VIEW_H

#include "PositionView.h"

class RestView
{
private:
    int pieceId;
    PositionView position;
    long long startTime;
    long long endTime;

public:
    RestView();

    RestView(int pieceId,
             const PositionView& position,
             long long startTime,
             long long endTime);

    int getPieceId() const;

    const PositionView& getPosition() const;

    long long getStartTime() const;
    long long getEndTime() const;

    double getProgress(long long currentTime) const;
};

#endif
