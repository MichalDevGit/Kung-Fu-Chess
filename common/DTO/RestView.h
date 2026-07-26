#ifndef REST_VIEW_H
#define REST_VIEW_H

#include <nlohmann/json.hpp>

#include "PositionView.h"
#include "../enums/RestKind.h"

class RestView
{
private:
    int pieceId;
    PositionView position;
    long long startTime;
    long long endTime;
    RestKind kind;

public:
    RestView();

    RestView(int pieceId,
             const PositionView& position,
             long long startTime,
             long long endTime,
             RestKind kind);

    int getPieceId() const;

    const PositionView& getPosition() const;

    long long getStartTime() const;
    long long getEndTime() const;

    RestKind getKind() const;

    double getProgress(long long currentTime) const;

    nlohmann::json toJson() const;
    static RestView fromJson(const nlohmann::json& j);
};

#endif
