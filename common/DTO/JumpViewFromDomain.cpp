#include "JumpView.h"

// See PositionViewFromDomain.cpp for why this converting constructor lives in
// its own translation unit, separate from JumpView.cpp: it's the only
// JumpView code referencing the server-side domain Jump type.
JumpView::JumpView(const Jump& jump)
    : active(jump.isActive()),
      position(jump.getPosition()),
      startTime(jump.getStartTime()),
      endTime(jump.getEndTime())
{
}
