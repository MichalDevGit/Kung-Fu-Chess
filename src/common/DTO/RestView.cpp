#include "RestView.h"
#include "../TimeProgress.h"

RestView::RestView()
    : pieceId(-1),
      position(0, 0),
      startTime(0),
      endTime(0),
      kind(RestKind::Long)
{
}

RestView::RestView(int pieceId,
                   const PositionView& position,
                   long long startTime,
                   long long endTime,
                   RestKind kind)
    : pieceId(pieceId),
      position(position),
      startTime(startTime),
      endTime(endTime),
      kind(kind)
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

RestKind RestView::getKind() const
{
    return kind;
}

double RestView::getProgress(long long currentTime) const
{
    return computeProgress(startTime, endTime, currentTime);
}
