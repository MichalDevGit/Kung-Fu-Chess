#ifndef REST_H
#define REST_H

class Rest
{
private:
    int pieceId;
    long long startTime;
    long long endTime;

public:
    Rest();

    Rest(
        int pieceId,
        long long startTime,
        long long endTime);

    int getPieceId() const;

    long long getStartTime() const;

    long long getEndTime() const;

    bool isFinished(long long currentTime) const;
};

#endif
