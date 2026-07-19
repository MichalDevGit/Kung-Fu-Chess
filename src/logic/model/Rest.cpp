#include "Rest.h"

Rest::Rest()
    : pieceId(-1),
      startTime(0),
      endTime(0)
{
}

Rest::Rest(
    int pieceId,
    long long startTime,
    long long endTime)
    : pieceId(pieceId),
      startTime(startTime),
      endTime(endTime)
{
}

int Rest::getPieceId() const
{
    return pieceId;
}

long long Rest::getStartTime() const
{
    return startTime;
}

long long Rest::getEndTime() const
{
    return endTime;
}

bool Rest::isFinished(long long currentTime) const
{
    return currentTime >= endTime;
}
