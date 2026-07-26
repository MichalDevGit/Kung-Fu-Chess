#include "PositionView.h"

// Split into its own translation unit, separate from PositionView.cpp,
// deliberately: this is the only PositionView code that references the
// server-side domain Position type. A consumer that never constructs a
// PositionView from a real Position (e.g. a future client, which only ever
// builds these from JSON) never triggers the linker to pull this .obj out of
// the static library, so it never needs Position.cpp's symbols either --
// unlike PositionView's other constructors/toJson()/fromJson(), which live in
// PositionView.cpp and have no such dependency.
PositionView::PositionView(const Position& position)
    : row(position.getRow()),
      col(position.getCol())
{
}
