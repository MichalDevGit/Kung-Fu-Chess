#ifndef REST_H
#define REST_H

#include "../../common/enums/RestKind.h"

class Rest
{
private:
    int pieceId;
    long long startTime;
    long long endTime;
    RestKind kind;

public:
    Rest();

    Rest(
        int pieceId,
        long long startTime,
        long long endTime,
        RestKind kind);

    int getPieceId() const;

    long long getStartTime() const;

    long long getEndTime() const;

    RestKind getKind() const;

    bool isFinished(long long currentTime) const;
};

#endif
