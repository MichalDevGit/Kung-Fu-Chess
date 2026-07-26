#ifndef GAME_VIEW_H
#define GAME_VIEW_H

#include <vector>

#include <nlohmann/json.hpp>

#include "BoardView.h"
#include "MotionView.h"
#include "JumpView.h"
#include "RestView.h"
#include "PositionView.h"

class GameView
{
private:
    BoardView board;
    MotionView motion;
    JumpView jump;
    std::vector<RestView> rests;
    bool hasSelection;
    PositionView selectedPosition;
    long long currentTime;

public:
    GameView();

    GameView(const BoardView& board,
             const MotionView& motion,
             const JumpView& jump,
             const std::vector<RestView>& rests,
             bool hasSelection,
             const PositionView& selectedPosition,
             long long currentTime);

    const BoardView& getBoard() const;
    const MotionView& getMotion() const;
    const JumpView& getJump() const;
    const std::vector<RestView>& getRests() const;
    bool getHasSelection() const;
    const PositionView& getSelectedPosition() const;
    long long getCurrentTime() const;

    nlohmann::json toJson() const;
    static GameView fromJson(const nlohmann::json& j);
};

#endif
