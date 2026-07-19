#include "RestView.h"
#include "../TimeProgress.h"

RestView::RestView()
    : pieceId(-1),
      position(0, 0),
      startTime(0),
      endTime(0)
{
}

RestView::RestView(int pieceId,
                   const PositionView& position,
                   long long startTime,
                   long long endTime)
    : pieceId(pieceId),
      position(position),
      startTime(startTime),
      endTime(endTime)
{
}

int RestView::getPieceId() const
{
    return pieceId;
}

const PositionView& RestView::getPosition() const
{
    return position;
}

long long RestView::getStartTime() const
{
    return startTime;
}

long long RestView::getEndTime() const
{
    return endTime;
}

double RestView::getProgress(long long currentTime) const
{
    return computeProgress(startTime, endTime, currentTime);
}
