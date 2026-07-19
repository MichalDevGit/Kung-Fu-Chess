# KungFuChess — Architecture Overview

This document is a standing reference for AI assistants (and humans) working on this
codebase. It describes how the project is currently built, not how it should
eventually look. Read this before exploring the source tree — it should make a full
re-read of `src/` unnecessary for most tasks. If you change the architecture in a way
that makes a section below wrong, update that section in the same change.

Last reviewed: 2026-07-19, against the source tree as of commit `02cd0a6`, updated to
reflect: per-piece post-move `Rest` in `RealTimeArbiter`/`GameEngine`; the new
`GameView` aggregate DTO (which **replaces** `Controller::getBoardView()`) carrying
`MotionView`/`JumpView`/`RestView`/selection/current-time snapshots to the UI; the
traveling-piece animation, selection highlight, jump highlight and rest-countdown
rendering in `Renderer`/`BoardCanvas`; right-click-to-jump input in `GameLoop`; a
fourth `PieceState::Jump` value with a real sprite-based jump animation (not just an
overlay highlight), driven by the new `UI/AnimationFrame` collaborator; and the new
`common/enums/PieceStateToString.h` free function that centralizes the
`PieceState`-to-sprite-folder-name mapping (used by `SpriteManager`).

## What this project is

"KungFuChess" is a real-time chess variant (no turns — in the classic Kung Fu Chess
rules, pieces move independently with per-piece cooldowns) implemented in **C++17**,
rendered with **OpenCV**, built with **CMake**. It is an early-stage / in-progress
codebase: the domain logic (rules, move validation, real-time timing) is fairly well
developed and unit-tested, and — as of this review — it **is** wired into a live
graphical executable via `GameFactory` (initial board setup) and `UI/GameLoop` (the
render/input loop). See "Current state of integration" below for exactly what that
loop does and doesn't do yet (no animations, no cooldown visuals).

## Build system

- Root `CMakeLists.txt` globs all `.cpp` under `src/` and splits them into two targets:
  - `KungFuChess` — the game executable. Everything under `src/` except `src/tests/`.
  - `KungFuChess_tests` — doctest-based test executable. Everything under
    `src/tests/` (except `src/tests/main.cpp`) plus all the same logic/UI sources
    (`LIB_SOURCES`) minus `src/main.cpp`.
- `src/tests/main.cpp` is a **third**, standalone entry point (a console REPL — see
  below). As of this review it is not included in either CMake target's source list
  (`GAME_SOURCES` excludes all of `src/tests/`, and `TEST_SOURCES` explicitly excludes
  this file), so it currently isn't built by default. If you need the REPL, check
  whether it's been wired into a target before assuming `cmake --build` produces it.
- OpenCV is vendored under `lib/` (headers in `lib/include/opencv2`, prebuilt libs/DLLs
  in `lib/bin`), version 4.5.1. Debug links `opencv_world451d`, Release links
  `opencv_world451`. DLLs are copied next to each executable post-build.
- C++ standard: 17.

## Module map and dependency direction

```
common/enums        <-- no dependencies (PieceColor, PieceType, PieceState, MoveValidationReason)
common/PixelPosition <-- no dependencies ({int x, y} value type, shared by UI and logic/Controller)
        ^
common/DTO           -- View/DTO objects for rendering; depends on logic/model
        ^
logic/model          -- core domain classes; depends only on common/enums
        ^
logic/rules           -- per-piece legality; depends on logic/model + common/DTO/MoveValidation.h
        ^
logic/Engine          -- GameEngine; depends on logic/model, logic/rules, logic/Controller/RealTimeArbiter
        ^
logic/Controller      -- Controller, RealTimeArbiter, BoardMapper; depends on logic/Engine, logic/model,
                          common/PixelPosition (input) and common/DTO/BoardView (output snapshot)
        ^
logic/IO              -- BoardParser, BoardPrinter, CommandProcessor, GameFactory; depends on all of the above
        ^
UI/                   -- OpenCV rendering + the live loop; depends on common/DTO + common/enums +
                          common/PixelPosition, and — only via UI/GameLoop, only on
                          logic/Controller/Controller — never directly on logic/Engine/model/rules
```

Note: `logic/Controller` depending on `common/DTO/BoardView` (for `Controller::getBoardView()`) and
`common/PixelPosition` (for `Controller::handlePixelClick()`) does not create a cycle — `common/*`
still depends only downward on `logic/model`. `PixelPosition` lives in `common/` rather than `UI/`
specifically so both `UI/` and `logic/Controller` can depend on it without either depending on the other.

`UI/GameLoop` is the one deliberate exception to "UI never depends on logic/*": it holds a
`Controller&` and calls `handlePixelClick`/`getBoardView`/`isGameOver`/`wait` on it — exactly the
sanctioned UI↔Controller boundary, and nothing else in `UI/` reaches any further into `logic/*`.
`GameFactory` (in `logic/IO`, not `logic/Engine`) is what `main.cpp` calls to get a fully-populated
`GameEngine` to hand to `Controller`; `GameEngine` itself does not depend on `logic/IO`.

Important nuance: `common/DTO` is meant to decouple the UI from the logic model, but
its converting constructors (`BoardView(const Board&)`, `PieceView(const Piece&)`,
`PositionView(const Position&)`) directly `#include` the `logic/model` headers. So the
decoupling is only "UI never includes logic/model", not "DTO is independent of
logic/model" — the two are still transitively coupled.

`logic/*` never includes anything from `UI/`. Good — the intended layering (logic
independent of rendering) does hold in that direction.

## Core domain model (`src/logic/model/`)

- **`Position`** — `{int row, col}` value type. Has `operator<` so it can go in
  `std::set<Position>` (used by rule legality sets).
- **`Motion`** — an in-flight move for the real-time engine: `{bool active; Position
  from, to; long long startTime, endTime;}`. `isFinished(now)` / `hasStarted(now)`.
- **`Jump`** — same shape as `Motion` but for a stationary "jump" ability: `{bool
  active; Position position; long long startTime, endTime;}`. Represents a temporary
  ambush/counter window at a single square, not a move.
- **`Rest`** — `{int pieceId; long long startTime, endTime;}`. Represents a
  per-piece "just moved, can't move/jump again yet" cooldown. Unlike `Motion`/
  `Jump`, it's keyed by **piece id**, not `Position` (a piece stays put while
  resting, so identity — not square — is what a `requestMove`/`requestJump` guard
  needs to check), and it has no `active` flag: it lives in `RealTimeArbiter`'s
  `std::map<int, Rest>`, where map presence is itself the "active" signal. Lives in
  `RealTimeArbiter`, not `Board` — same ownership split as `Motion`/`Jump`.
- **`Piece`** — `{int id; PieceType type; PieceColor color; PieceState state;
  Position position;}`. `operator==`/`!=` compare **only by id**. `state` exists and
  is settable (`setState`), but no production code ever calls `setState` — see Gaps.
- **`Board`** — `{int rows, cols; std::vector<Piece> pieces;}`. **Not a 2D grid** —
  a flat, sparse vector of only-occupied squares; lookups are linear scans by
  position. `movePiece(from, to)` is a bare `setPosition` call with **no
  destination-occupancy check** — callers (`GameEngine::executeMove`) are responsible
  for removing whatever is at the destination first.
- **`GameState`** — `{Board board; bool gameOver;}`. No turn counter, no
  player-to-move field — consistent with the turnless real-time design.

Ownership: `GameState` owns `Board` by value → `Board` owns `Piece`s by value in a
vector → `Piece` owns `Position` by value. `Motion`/`Jump` are transient value objects
held only by `RealTimeArbiter`, not by `Piece`.

## Rules system (`src/logic/rules/`)

`IMovementRule` — single-method interface:
```cpp
virtual std::set<Position> legalDestinations(const Board& board, const Piece& piece) const = 0;
```
Six concrete implementations (`BishopRule`, `RookRule`, `QueenRule`, `KingRule`,
`KnightRule`, `PawnRule`) implement raw movement geometry — blocking, board bounds,
capture-vs-friendly — but **no check/checkmate detection, no castling, no en passant,
no stalemate**. Pawn promotion is handled elsewhere (in `GameEngine::executeMove`),
not in `PawnRule`.

`RuleEngine` owns one value-instance of each rule and a `std::map<PieceType, const
IMovementRule*>` built in its constructor (no heap allocation, no factory).
`validateMove(board, from, to)` checks bounds → source-empty → friendly-fire →
dispatch to the right rule → membership in `legalDestinations()`, returning a
`MoveValidation{bool isValid; MoveValidationReason reason;}`.

"Game over" is triggered **only** by an actual king capture inside
`GameEngine::executeMove`, never by check detection (there isn't any).

## Real-time ("kung fu") mechanics

`PieceState` (`common/enums/PieceState.h`) has 4 values: `Idle, Moving, Captured,
Jump`. This still does **not** fully match the on-disk sprite state folders
(`idle/move/jump/short_rest/long_rest` — see UI section: `Moving`/`Captured` map to
`"moving"`/`"captured"`, not `"move"`/anything-captured-shaped; only `Jump`'s mapping
to `"jump"` is correct), and no production code calls `Piece::setState` at all —
`Piece.state` itself is still never mutated. The `Jump` sprite state *is* now driven
in the live render loop, but — same as `Motion` before it — via `UI/AnimationFrame`
reading `GameView`'s `JumpView` directly, not via `Piece.state`.

`RealTimeArbiter` (`logic/Controller/RealTimeArbiter`) is a scheduler with a manually
advanced logical clock (`long long currentTime`, moved forward only by explicit
`advanceTime(ms)` calls — **no threads, no wall-clock, no real tick loop anywhere in
the codebase**). Important simplification: it tracks only **one global `Motion` and
one global `Jump`** for the entire board — not per-piece. Only one piece on the whole
board can be moving (or jumping) at a time; a second `requestMove`/`requestJump` while
one is active is rejected with `MoveAlreadyInProgress`. This is a significant MVP-level
simplification versus the classic Kung Fu Chess variant, where every piece has an
independent cooldown.

`GameEngine` (`logic/Engine/GameEngine`) drives the simulation:
- `MILLIS_PER_SQUARE = 1000` — a move's duration is `pathLength * 1000ms`, where
  `pathLength` depends on piece type (King/Knight = 1; Pawn/Bishop = row distance;
  Rook = Manhattan distance; Queen = Manhattan if straight else row distance).
- `REST_DURATION_MILLIS = 2000` — after a piece finishes a move (see
  `settleCompletedMotions` below), that specific piece is put into a `Rest` for this
  long, during which `requestMove`/`requestJump` reject it specifically with
  `MoveValidationReason::PieceResting` (checked *after* the existing global
  `MoveAlreadyInProgress`/rule-validation/existence gates, right before the
  state-mutating call — same "global gates first, piece-specific gates last"
  ordering in both methods). This is a **per-piece** lock layered on top of the
  still-unchanged single-global-`Motion`/`Jump` exclusivity below — it closes part
  of gap #5 (per-piece rest specifically), not gap #5 as a whole; only one piece can
  still be *traveling* at a time board-wide.
- `requestMove(from, to)` — rejects if game over, a motion is already in progress, or
  (once validation and the piece lookup both succeed) the piece is resting;
  otherwise validates via `RuleEngine` and starts a `Motion` in the arbiter. **No
  board mutation happens at request time.**
- `requestJump(position)` — same guards (plus the same resting check, ordered after
  `hasActiveJump()`), starts a `Jump` of duration `MILLIS_PER_SQUARE` at the piece's
  own square. Doesn't move the piece; models a temporary ambush/counter window.
- `executeMove(motion)` — runs when a motion *completes* (not when requested), and is
  **not** aware of `Rest` at all (rest scheduling is a settlement-timing concern that
  belongs in `settleCompletedMotions`, not in the pure board-mutation function):
  - Special case: if a `Jump` is active at the motion's destination and the mover is
    the opposite color of the jumping piece, the **mover gets captured** (removed) and
    the jump is consumed — the move itself doesn't happen. This is how "jump" acts as
    an ambush/counter mechanic.
  - Otherwise: capture whatever occupies the destination (game-over if it was a King),
    `board.movePiece(from, to)`, then check pawn promotion (reaching the far row →
    `setType(Queen)`).
- `advanceTime(ms)` — advances the arbiter's clock, then settles any motion/jump whose
  `endTime` has passed (`settleCompletedMotions` calls `executeMove` +
  `finishMotion`; `settleCompletedJumps` just deactivates — the jump's actual capture
  effect is checked eagerly inside a *different* motion's `executeMove`, not here),
  then `settleCompletedRests()` (purges any `Rest` whose `endTime` has passed).
  `settleCompletedMotions` captures the mover's id from `motion.getFrom()` **before**
  calling `executeMove`, then re-looks it up by id (`Board::getPieceById`)
  *afterward* and only starts a `Rest` for it if it's still alive **and** now sitting
  at `motion.getTo()`. This matters: in the ambush-capture special case above, the
  *mover* is removed but the *defending* piece is left standing at that square —
  naively checking "is there a piece at `motion.getTo()`?" after `executeMove` would
  find the defender and incorrectly put *it* to rest for successfully defending.
  Keying the lookup by the mover's id specifically avoids that.

There is currently no code path that calls `advanceTime` on a real clock/timer — time
only moves forward when a caller (the REPL's `wait <ms>` command, or a test) asks it
to.

## Controller & command flow

`Controller` (`logic/Controller/Controller`) implements click-to-select/click-to-move
and is the sole mediator between UI and logic — the UI never touches `GameEngine`
directly. It holds `GameEngine&`, a `BoardMapper` (by value), `hasSelection`,
`selectedPosition`.

- `click(Position)`: selects a same-color piece on first click (ignores clicks on
  empty squares), re-selects if a second same-color piece is clicked, otherwise calls
  `requestMove` and clears selection regardless of validation outcome. Still public
  and used directly by the REPL (`CommandProcessor`) and unit tests, which already
  work in logical-position space.
- `handlePixelClick(const PixelPosition&)` — the UI-facing entry point for
  selecting/moving. Translates via `boardMapper.pixelToCell(...)` into a `Position`,
  then delegates to `click(Position)`.
- `handlePixelJump(const PixelPosition&)` — the UI-facing entry point for jumping,
  mirroring `handlePixelClick`: translates the pixel via the same `boardMapper`,
  then delegates to `jump(Position)`. Wired to right-click in `UI/GameLoop` (see
  below) — before this existed, nothing in the graphical game could ever trigger a
  jump at all.
- `jump(Position)` → `requestJump`. `wait(ms)` → `advanceTime`. `printBoard(ostream&)`
  → `BoardPrinter::print`.
- `getGameView() const` → builds and returns a `GameView`, the single per-frame
  snapshot the UI polls (**replaces** the old `getBoardView()`/`BoardView`-only
  method). Assembles: `BoardView(gameEngine.getGameState().getBoard())`; a
  `MotionView`/`JumpView` (default-constructed/inactive if `GameEngine` reports no
  active motion/jump); a `std::vector<RestView>` built by mapping
  `gameEngine.getActiveRests()` through `Board::getPieceById` to resolve each
  resting piece's current position (skipping any whose piece id no longer exists —
  e.g. captured mid-rest); and the controller's own `hasSelection`/
  `selectedPosition`. `GameView` is a new sibling DTO alongside `BoardView`, not an
  extension of it — `Motion`/`Jump`/`Rest` live in `RealTimeArbiter`, not `Board`,
  and selection is Controller-owned UI state with no `Board`/`GameState`
  representation at all, so neither can be folded into `BoardView(const Board&)`'s
  single-source conversion. (This supersedes an earlier note in this file that
  anticipated `getBoardView()` itself absorbing in-motion data — the real shape
  needed more than `Board` alone could express.)
- `isGameOver() const` → forwards to `gameEngine.getGameState().isGameOver()`. Lets
  `UI/GameLoop` decide when to stop the render loop without reaching past `Controller`
  into `GameEngine` directly.

`BoardMapper` (`logic/Controller/BoardMapper`) — `pixelToCell(int x, int y)` and an
overload `pixelToCell(const PixelPosition&)`, both dividing by a hardcoded
`CELL_SIZE = 100`. This constant is still **independently duplicated** in
`UI/CoordinateConverter::CELL_SIZE` and again as a constructor parameter in
`BoardCanvas` — three places, one source of truth would be safer (not yet fixed).

`PixelPosition` (`common/PixelPosition`) — a plain `{int x, y}` value type. It lives in
`common/`, not `UI/`, specifically so `Controller::handlePixelClick` can accept it
without `logic/Controller` depending on `UI/`.

Full intended flow:
- **Input**: UI captures a mouse click as a `PixelPosition` → `Controller::handlePixelClick`
  → `BoardMapper::pixelToCell` → `Controller::click` (select, then target) →
  `GameEngine::requestMove` → `RuleEngine::validateMove` → on success,
  `RealTimeArbiter::startMotion` → later, an explicit `advanceTime` tick →
  `GameEngine::executeMove` mutates `Board`/`GameState`.
- **Rendering**: UI calls `Controller::getBoardView()` every frame → `BoardView(const
  Board&)` snapshot → UI renders whatever it gets, with no knowledge of whether the
  board is static or (in future) mid-animation.

## DTO layer (`src/common/DTO/`)

`PositionView`, `PieceView`, `BoardView` mirror `Position`/`Piece`/`Board` for
rendering, each with a converting constructor from the logic-layer type.
`MoveValidation` (header-only) is `{bool isValid; MoveValidationReason reason;}`.

`MotionView`/`JumpView` mirror `Motion`/`Jump` the same way (converting constructor,
default constructor for the "inactive" case). `RestView` is the exception — since
`Rest` only carries a piece id (no `Position`), `RestView` has **no** converting
constructor from `Rest` alone; it's built manually by `Controller::getGameView()`
after resolving the resting piece's current position via `Board::getPieceById`.
All three add `getProgress(long long currentTime) const`, an elapsed-fraction
`[0.0, 1.0]` computed via the shared free function `computeProgress` in
`common/TimeProgress.h` (one clamp/divide implementation, reused three times,
instead of copy-pasting it into each DTO).

`GameView` is a new aggregate "one render frame's worth of data" DTO: `{BoardView
board; MotionView motion; JumpView jump; std::vector<RestView> rests; bool
hasSelection; PositionView selectedPosition; long long currentTime;}`, built
entirely by `Controller::getGameView()` (see Controller section above for why this
is a sibling of `BoardView` rather than an extension of it).

`BoardView(const Board&)` now loops `row`/`col` and calls `board.getPiece(Position(row,
col))` for each cell, producing a dense, row-major 64-slot vector — empty squares get a
default `PieceView(PieceType::Empty, PieceColor::None, ...)` at that position — so it's
consistent with `BoardView::getPiece(row, col)`'s `pieces[row*cols+col]` indexing.
(Previously this constructor iterated `Board::getPieces()`, a sparse, insertion-ordered
vector of only occupied squares, which produced wrong results when combined with
`getPiece(row,col)` — fixed as part of wiring up what was then `Controller::getBoardView()`
and is now folded into `Controller::getGameView()`'s `BoardView` field.)

## IO layer (`src/logic/IO/`)

- `GameFactory::createNewGame()` — the production entry point for starting a game.
  Returns a fully-populated `GameEngine` as a single prvalue (`return
  GameEngine(GameState(createClassicBoard()));`) — deliberately not through a named
  local, so C++17's *guaranteed* copy elision applies. This matters because
  `GameEngine`'s `RuleEngine` member stores raw pointers to its own rule sub-objects
  (see Rules section above); an actual copy/move of a `GameEngine` would leave those
  pointers referring to the wrong object's memory. `createClassicBoard()` (private)
  builds the standard starting position by directly constructing and placing all 32
  `Piece` objects on an 8x8 `Board` — no string parsing involved.
- `BoardParser::parse(text)` — static, parses whitespace-separated `<color><TYPE>`
  tokens (e.g. `wK`, `bQ`, `.` for empty) into a `Board`. Row width must be consistent
  or it throws. Piece IDs come from a process-global static counter (not reset per
  parse — fine for uniqueness, not reproducible across repeated parses in one run).
  **Not used by `GameFactory`/the graphical executable** — text-based board setup is
  kept as a REPL/testing convenience, deliberately separate from the production path,
  and may be deprecated later.
- `BoardPrinter::print(board)` — static, row-major space-separated `Piece::toString()`
  output, inverse of the parser's format.
- `CommandProcessor::run()` — a stdin-driven REPL: reads board text until a
  `"Commands:"` line, parses it via `BoardParser`, builds `GameState`/`GameEngine`/
  `Controller`, then dispatches commands: `click x y`, `wait <ms>`, `print board`,
  `jump x y`. Still useful for exercising the logic stack from the console without
  OpenCV, but no longer the only place that runs the full stack end-to-end now that
  `UI/GameLoop` exists. Its entry point (`src/tests/main.cpp`) is presently not
  attached to a CMake target (see Build system).

Two on-disk data formats exist but are **not read by any current C++ code**:
- `assets/pieces1/board.csv` — 8x8 CSV, `<TYPE><COLOR>` per cell (reverse order from
  `BoardParser`'s `<color><TYPE>` text format).
- `assets/pieces*/*/states/*/config.json` — `{"physics": {"speed_m_per_sec", "next_state_when_finished"}, "graphics": {"frames_per_sec", "is_loop"}}`. Looks like planned
  animation/state-machine metadata for the cooldown-state rendering that doesn't exist
  yet (no JSON library is even linked in `CMakeLists.txt`).

## UI / rendering layer (`src/UI/`)

- `Img` — RAII `cv::Mat` wrapper. `read()` loads via `cv::imread(..., IMREAD_UNCHANGED)`
  (preserves alpha). `draw_on()` alpha-blends a 4-channel image onto another at (x,y).
  `draw_rectangle(x, y, w, h, color, thickness)` wraps `cv::rectangle` — a **direct,
  opaque** write (same category as `put_text`, unlike `draw_on`'s alpha-blend
  compositing); pass `thickness = cv::FILLED` for a filled rectangle. Used by
  `BoardCanvas` for selection/jump highlights (outlined) and the rest-countdown bar
  (filled) — one primitive, three callers, all rendering as **solid colors**, not
  translucent overlays (a `cv::rectangle` limitation, accepted as an MVP
  simplification). `show()` is a pure drawing call — `cv::imshow(windowName(), img)`
  only, no `cv::waitKey` and no return value. Pumping the window's event queue /
  reading a keypress is deliberately **not** this class's job (see `GameLoop` below) —
  `Img`/`BoardCanvas`/`Renderer` only ever produce pixels, never interpret input.
  `windowName()` is a shared static constant (`"KungFuChess"`) so other code (namely
  `GameLoop`, for `cv::setMouseCallback`) can address the same window `show()` draws to.
- `PixelPosition` — moved to `common/PixelPosition` (see module map) — `{int x, y}`,
  pixel-space analog of `Position`.
- `CoordinateConverter` — cell→pixel via a duplicated `CELL_SIZE = 100`. **Currently
  unused** — `Renderer`/`BoardCanvas` do their own inline pixel math instead.
- `BoardCanvas` — holds a pristine `background` `Img` (loaded once from the board
  image) plus a `frame` `Img` (the actual draw buffer) and `cellSize`. `beginFrame()`
  resets `frame = background` — this exists because `drawPiece`/`drawPieceAtPixel`
  permanently blend piece sprites into whatever image they're given; without
  resetting to a clean background before each frame, pieces would leave visible
  trails as they move across repeated `render()` calls in the live loop.
  `drawPiece(Img&, row, col)` now just delegates to `drawPieceAtPixel(Img&, const
  PixelPosition&)` (added so a moving piece can be drawn at an arbitrary
  interpolated pixel, not just a cell). `getCellPosition(row, col)` is yet another
  independent cell→pixel calculation. `getInterpolatedPosition(from, to, progress)`
  linearly interpolates between two cells' pixel positions — this is what makes the
  traveling-piece animation possible. `drawSelectionHighlight(row, col)` and
  `drawJumpHighlight(row, col)` both go through a private `drawCellOutline(row, col,
  color, thickness)` helper (different named `static const cv::Scalar`/`static
  constexpr int` colors/thicknesses so the two are visually distinct).
  `drawRestProgress(row, col, progress)` draws a filled bar along the cell's bottom
  edge whose width **shrinks** as the rest counts down (`cellSize * (1 -
  progress)`) — `getProgress()`'s elapsed-fraction convention is inverted to a
  remaining-time convention here, in `BoardCanvas`, not baked into `RestView`, since
  `MotionView`/`JumpView` need the elapsed-fraction semantics as-is for
  interpolation. `getWindowName()` forwards to `Img::windowName()`.
- `SpriteManager` — builds sprite file paths as
  `{assets}/{piecesFolder}/{TYPE}{COLOR}/states/{state}/sprites/{frame}.png`.
  `getPieceSprite` caches **decoded** `Img`s in `spriteCache` (keyed by
  type+color+state+frame), not just path strings — each unique sprite is read
  and resized from disk exactly once; every later call (every piece, every
  frame, in the live render loop) returns a cheap in-memory `Img` copy
  (`cv::Mat::clone()`, no disk I/O/decode). This matters a lot once pieces
  animate: the old path-only cache re-read+re-decoded every visible sprite from
  disk on every single frame, which was the actual bottleneck behind visibly
  stuttery motion (the render *loop* was already running every iteration — each
  individual `render()` call was just slow). State-to-folder-name conversion is no
  longer a private `SpriteManager` method — it now calls the free function
  `pieceStateToString` (`common/enums/PieceStateToString.h`), which is what
  `getPath`/`getPieceSprite`'s cache key both call through. **Still out of sync with
  the actual on-disk folder names for two of the four values**: it emits
  `"moving"`/`"captured"` but the folders are `idle/move/jump/short_rest/long_rest` —
  requesting a `Moving`-state sprite will still fail to load. Only `Idle → "idle"` and
  the new `Jump → "jump"` mapping are correct. `typeToString`'s default case and
  `colorToString`'s fallback also produce wrong-but-plausible values for
  `Empty`/`None` inputs rather than erroring (latent, since `Renderer` filters out
  `Empty` pieces before calling it).
- `AnimationFrame` — a small UI-layer collaborator (constructed with a `const
  BoardCanvas&`) that is the **only** place deciding, per board square, what to
  render: `resolve(const GameView&, row, col)` returns a `Resolution{PieceState
  state; int frame; PixelPosition position;}`. It checks whether that square is the
  `from` of an active `Motion` (→ `PieceState::Idle`, frame `1`, interpolated pixel
  position via `canvas.getInterpolatedPosition`) or the position of an active `Jump`
  (→ `PieceState::Jump`, a frame computed from `jump.getProgress(currentTime)` via a
  private non-looping `frameIndexForProgress(progress, frameCount)` helper — hardcoded
  `frameCount = 5` to match the on-disk sprite count, since no JSON parsing exists yet
  to read it from `config.json`, same MVP simplification as the rest of the codebase —
  and `canvas.getCellPosition(row, col)`), or neither (→ `PieceState::Idle`, frame `1`,
  `canvas.getCellPosition(row, col)`). This is what makes the jumping-piece sprite
  animation possible; `BoardCanvas` still owns the actual pixel arithmetic
  (`AnimationFrame` only decides *which* `BoardCanvas` method's result to use), and
  `SpriteManager` still owns turning `(PieceView, PieceState, frame)` into an `Img`.
- `Renderer::render(const GameView&)` — `canvas.beginFrame()`, then: iterates all
  cells, skips `Empty` squares, and for every occupied square asks
  `AnimationFrame::resolve(gameView, row, col)` for the `{state, frame, position}` to
  draw, fetches that sprite from `SpriteManager`, and draws it at that pixel position
  via `canvas.drawPieceAtPixel`. `Renderer` itself no longer contains any
  motion/jump-specific branching (no more "skip this cell, it's drawn separately
  below" special case) — that decision now lives entirely in `AnimationFrame`. After
  the per-square loop: if a jump is active, draws a jump highlight at its position;
  draws a rest-progress bar for every `RestView` in the snapshot; if there's a
  selection, draws a selection highlight; finally `canvas.show()`. Purely a drawing
  operation: `void`, no return value, no awareness that a "frame" is part of a loop or
  that input exists.
- `GameLoop` — the live game loop and the **only** place in `UI/` that touches
  input or `logic/*`. Constructed with `Controller&`, `Renderer&`, `BoardCanvas&`.
  `run()`: registers `cv::setMouseCallback` on `canvas.getWindowName()` once (a static
  trampoline casts `void* userdata` back to `GameLoop*` and dispatches on the event
  type: `EVENT_LBUTTONDOWN` → `onMouseDown` → `controller.handlePixelClick(PixelPosition(x,
  y))`; `EVENT_RBUTTONDOWN` → `onMouseRightDown` → `controller.handlePixelJump(PixelPosition(x,
  y))` — the only input path that can ever produce an active `Jump` in the live
  game), then loops: measure wall-clock delta via `std::chrono::steady_clock` →
  `controller.wait(deltaMs)` (this is what finally drives `GameEngine::advanceTime`
  off a real clock, instead of only a REPL/test calling it manually) →
  `renderer.render(controller.getGameView())` → `cv::waitKey(1)` (owned here, not in
  `Renderer`/`Img`, since interpreting a keypress is an input decision) → stops the
  loop on ESC or `controller.isGameOver()`.

## Current state of integration

The logic layer is now wired into the graphical executable:
`main.cpp` calls `GameFactory::createNewGame()` → `Controller` → constructs
`BoardCanvas`/`SpriteManager`/`AnimationFrame`/`Renderer` → hands them plus the
`Controller` to a `GameLoop` and calls `run()`. This is now the only entry point
needed to play a game with a live board, mouse input, and real-time piece movement.

What's still missing/rough:
- **Sprite-level animation exists only for `Jump`** — `AnimationFrame` swaps to the
  real `PieceState::Jump` sprite frames (5 frames, non-looping) while a jump is
  active. `Motion` (traveling pieces) and the `Rest` cooldowns still render via
  `PieceState::Idle` frame `1` plus `BoardCanvas` overlay primitives (interpolated
  position, colored outlines, a progress bar) rather than their own sprite states —
  `SpriteManager`'s `pieceStateToString` mapping for `Moving`/`Captured` is still out
  of sync with the on-disk folder names (see gaps list), so a `Moving`-state sprite
  request would still fail to load if one were ever made.
- No visual feedback for game-over — the loop just stops; nothing is drawn to signal
  who won or that the window is about to close.
- The text-only REPL (`CommandProcessor` / `src/tests/main.cpp`) still exists
  side-by-side as a console-only way to exercise the logic stack, and (as of this
  review) isn't attached to a CMake build target.

## Testing (`src/tests/`)

doctest framework (single header, `src/tests/doctest.h`), driven by
`src/tests/test_main.cpp` (`DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN`). Tests mirror the
`logic/` structure under `src/tests/logic_tests/{controller,engine,io,model,rules}/`.
Pattern: one `TEST_CASE` per class with `SUBCASE`s per scenario; no fixtures, no
mocking (everything is tested against real concrete objects). Notably **no tests for
`src/UI/*`**, no tests for `CoordinateConverter`, and no tests for `common/DTO/*`
conversion constructors, including `BoardView(const Board&)` — worth adding coverage
for now that it's relied on by `Controller::getBoardView()`.

## Known gaps / things to be careful about when editing

1. ~~Logic layer isn't wired into the graphical executable~~ — fixed: `GameFactory` +
   `UI/GameLoop` now provide a live, playable loop (see "Current state of integration").
2. ~~`BoardView(const Board&)` produces a wrongly-ordered/sparse vector~~ — fixed: it
   now builds a dense, row-major vector consistent with `getPiece(row, col)`'s indexing.
3. `PieceState` enum (4 values: `Idle, Moving, Captured, Jump`) still doesn't cover the
   on-disk cooldown states `short_rest`/`long_rest` that the real-time mechanic needs
   for visual feedback, and `pieceStateToString` (`common/enums/PieceStateToString.h`,
   used by `SpriteManager`) still doesn't match on-disk folder names for `Moving`
   (`"moving"` vs. on-disk `"move"`). Only `Idle`/`Jump` are correct as of the jump
   animation work.
4. `Piece::setState` is never called outside tests — no production code marks a piece
   as `Moving`/`Captured`.
5. `RealTimeArbiter` supports only one in-flight motion/jump globally, not per-piece —
   a deliberate MVP simplification worth knowing about before extending it.
   **Partially addressed for post-move cooldowns**: `Rest` *is* per-piece (keyed by
   piece id, independent per piece, doesn't block other pieces), so multiple pieces
   can be resting simultaneously. The `Motion`/`Jump` themselves are still
   single-global exactly as before — only one piece can be *traveling*/*jumping* at
   a time board-wide; this gap is not fixed for those.
6. Cell-pixel size (`100`) is hardcoded independently in three places
   (`BoardMapper`, `CoordinateConverter`, `BoardCanvas`'s constructor arg in
   `main.cpp`) with no shared constant.
7. `config.json` (animation/physics metadata) and `board.csv` exist on disk but are
   parsed by no current code (no JSON library is even linked).
8. No check/checkmate/castling/en-passant/stalemate — only raw movement legality plus
   king-capture-ends-game.
9. `src/tests/main.cpp` (the REPL entry point) is not attached to any current CMake
   target — confirm before assuming `cmake --build` produces it.
10. Dead code: a large commented-out earlier `Img::draw_on` implementation still sits
    in `UI/img.cpp`.

## Where to look for X

| Task | Start here |
|---|---|
| Change/add a piece's movement rule | `src/logic/rules/<Piece>Rule.cpp`, registered in `RuleEngine`'s constructor |
| Change move/jump timing (cooldowns, speed) | `src/logic/Engine/GameEngine.cpp` (`MILLIS_PER_SQUARE`, `calculatePathLength`), `src/logic/Controller/RealTimeArbiter.cpp` |
| Change the post-move rest/cooldown duration or its guard | `src/logic/Engine/GameEngine.cpp` (`REST_DURATION_MILLIS`, `settleCompletedMotions`, the `isPieceResting` checks in `requestMove`/`requestJump`), `src/logic/Controller/RealTimeArbiter.cpp` (`startRest`/`isPieceResting`/`getActiveRests`/`purgeExpiredRests`) |
| Change what happens when a move completes (capture, promotion, game-over) | `GameEngine::executeMove` |
| Change click/selection UX | `src/logic/Controller/Controller.cpp` |
| Parse/print board text (REPL only, not production setup) | `src/logic/IO/BoardParser.cpp` / `BoardPrinter.cpp` |
| Change the initial/starting board setup | `src/logic/IO/GameFactory.cpp` (`createClassicBoard`) |
| Change the live loop, mouse handling, or when the game stops | `src/UI/GameLoop.cpp` |
| Change the per-frame data the UI renders from | `src/logic/Controller/Controller.cpp` (`getGameView`), `src/common/DTO/GameView.h`/`MotionView.h`/`JumpView.h`/`RestView.h` |
| Change the traveling-piece animation, jumping-piece animation, selection/jump highlights, or rest-progress bar | `src/UI/AnimationFrame.cpp` (which state/frame/position a square renders), `src/UI/Renderer.cpp` (draw order for the overlays), `src/UI/BoardCanvas.cpp` (pixel math, colors/thicknesses) |
| Fix/extend sprite rendering | `src/common/enums/PieceStateToString.h` (state-name mismatch for `Moving`/`Captured`), `src/UI/SpriteManager.cpp` (path/cache-key building), `src/UI/AnimationFrame.cpp` (which state/frame a piece requests) |
| Change how pixel clicks map to board cells | `src/logic/Controller/BoardMapper.cpp`, fed via `Controller::handlePixelClick`/`handlePixelJump` |
| Add/adjust tests | `src/tests/logic_tests/<matching-folder>/`, `src/tests/common_tests/` |
