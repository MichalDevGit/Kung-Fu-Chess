#ifndef REALTIMEARBITER_H
#define REALTIMEARBITER_H

#include <map>
#include <vector>

#include "../model/Motion.h"
#include "../model/Jump.h"
#include "../model/Rest.h"

class RealTimeArbiter
{
private:

    Motion currentMotion;
    long long currentTime;

    Jump currentJump;

    std::map<int, Rest> restsByPieceId;

public:
    RealTimeArbiter();

    bool hasActiveMotion() const;

    void startMotion(
        const Position& from,
        const Position& to,
        long long duration);

    void finishMotion();

    const Motion& getCurrentMotion() const;

    void advanceTime(long long milliseconds);

    long long getCurrentTime() const;

    bool shouldFinishCurrentMotion() const;


    void startJump(
        const Position& position,
        long long duration);
    
    void finishJump();
    
    bool hasActiveJump() const;
    
    const Jump& getCurrentJump() const;
    
    bool shouldFinishCurrentJump() const;


    void startRest(int pieceId, long long duration, RestKind kind);

    bool isPieceResting(int pieceId) const;

    std::vector<Rest> getActiveRests() const;

    void purgeExpiredRests();
};

#endif