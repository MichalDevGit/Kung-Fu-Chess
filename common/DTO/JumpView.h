#ifndef JUMP_VIEW_H
#define JUMP_VIEW_H

#include <nlohmann/json.hpp>

#include "PositionView.h"

#include "../../server/src/game/model/Jump.h"

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

    // All-fields constructor, used to reconstruct a JumpView from JSON on a
    // side (the client) that has no domain Jump object to convert from.
    JumpView(bool active,
             const PositionView& position,
             long long startTime,
             long long endTime);

    bool isActive() const;

    const PositionView& getPosition() const;

    long long getStartTime() const;
    long long getEndTime() const;

    double getProgress(long long currentTime) const;

    nlohmann::json toJson() const;
    static JumpView fromJson(const nlohmann::json& j);
};

#endif
