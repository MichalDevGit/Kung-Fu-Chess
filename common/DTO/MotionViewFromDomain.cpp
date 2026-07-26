#include "MotionView.h"

// See PositionViewFromDomain.cpp for why this converting constructor lives in
// its own translation unit, separate from MotionView.cpp: it's the only
// MotionView code referencing the server-side domain Motion type.
MotionView::MotionView(const Motion& motion)
    : active(motion.isActive()),
      from(motion.getFrom()),
      to(motion.getTo()),
      startTime(motion.getStartTime()),
      endTime(motion.getEndTime())
{
}
