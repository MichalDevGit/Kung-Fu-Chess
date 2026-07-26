#include "GameView.h"

GameView::GameView()
    : hasSelection(false),
      currentTime(0)
{
}

GameView::GameView(const BoardView& board,
                   const MotionView& motion,
                   const JumpView& jump,
                   const std::vector<RestView>& rests,
                   bool hasSelection,
                   const PositionView& selectedPosition,
                   long long currentTime)
    : board(board),
      motion(motion),
      jump(jump),
      rests(rests),
      hasSelection(hasSelection),
      selectedPosition(selectedPosition),
      currentTime(currentTime)
{
}

const BoardView& GameView::getBoard() const
{
    return board;
}

const MotionView& GameView::getMotion() const
{
    return motion;
}

const JumpView& GameView::getJump() const
{
    return jump;
}

const std::vector<RestView>& GameView::getRests() const
{
    return rests;
}

bool GameView::getHasSelection() const
{
    return hasSelection;
}

const PositionView& GameView::getSelectedPosition() const
{
    return selectedPosition;
}

long long GameView::getCurrentTime() const
{
    return currentTime;
}
