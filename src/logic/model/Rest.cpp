#include "Rest.h"

Rest::Rest()
    : pieceId(-1),
      startTime(0),
      endTime(0),
      kind(RestKind::Long)
{
}

Rest::Rest(
    int pieceId,
    long long startTime,
    long long endTime,
    RestKind kind)
    : pieceId(pieceId),
      startTime(startTime),
      endTime(endTime),
      kind(kind)
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

RestKind Rest::getKind() const
{
    return kind;
}

bool Rest::isFinished(long long currentTime) const
{
    return currentTime >= endTime;
}
