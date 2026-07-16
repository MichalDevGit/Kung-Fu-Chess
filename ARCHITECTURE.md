# KungFuChess — Architecture Overview

This document is a standing reference for AI assistants (and humans) working on this
codebase. It describes how the project is currently built, not how it should
eventually look. Read this before exploring the source tree — it should make a full
re-read of `src/` unnecessary for most tasks. If you change the architecture in a way
that makes a section below wrong, update that section in the same change.

Last reviewed: 2026-07-16, against the source tree as of commit `9b1b490`, updated
same-day to reflect the new `Controller::handlePixelClick`/`getBoardView` UI-mediation
API, the `PixelPosition` move to `common/`, and the `BoardView(const Board&)` fix.

## What this project is

"KungFuChess" is a real-time chess variant (no turns — in the classic Kung Fu Chess
rules, pieces move independently with per-piece cooldowns) implemented in **C++17**,
rendered with **OpenCV**, built with **CMake**. It is an early-stage / in-progress
codebase: the domain logic (rules, move validation, real-time timing) is fairly well
developed and unit-tested, but it is **not yet wired into the graphical executable** —
see "Current state of integration" below, this is the single most important thing to
know before making changes.

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
logic/IO              -- BoardParser, BoardPrinter, CommandProcessor; depends on all of the above

UI/                   -- OpenCV rendering; depends ONLY on common/DTO + common/enums + common/PixelPosition,
                          never on logic/* directly
```

Note: `logic/Controller` depending on `common/DTO/BoardView` (for `Controller::getBoardView()`) and
`common/PixelPosition` (for `Controller::handlePixelClick()`) does not create a cycle — `common/*`
still depends only downward on `logic/model`, and `UI/` still never depends on `logic/*`. `PixelPosition`
lives in `common/` rather than `UI/` specifically so both `UI/` and `logic/Controller` can depend on it
without either depending on the other.

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

`PieceState` (`common/enums/PieceState.h`) has exactly 3 values: `Idle, Moving,
Captured`. This does **not** match the on-disk sprite state folders
(`idle/move/jump/short_rest/long_rest` — see UI section), and no production code
transitions a `Piece`'s state at all — state-based rendering has no producer yet.

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
- `requestMove(from, to)` — rejects if game over or a motion is already in progress;
  otherwise validates via `RuleEngine` and starts a `Motion` in the arbiter. **No
  board mutation happens at request time.**
- `requestJump(position)` — same guards, starts a `Jump` of duration
  `MILLIS_PER_SQUARE` at the piece's own square. Doesn't move the piece; models a
  temporary ambush/counter window.
- `executeMove(motion)` — runs when a motion *completes* (not when requested):
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
  effect is checked eagerly inside a *different* motion's `executeMove`, not here).

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
- `handlePixelClick(const PixelPosition&)` — the UI-facing entry point. Translates via
  `boardMapper.pixelToCell(...)` into a `Position`, then delegates to `click(Position)`.
  This is the only method the UI needs to call to feed in raw mouse input.
- `jump(Position)` → `requestJump`. `wait(ms)` → `advanceTime`. `printBoard(ostream&)`
  → `BoardPrinter::print`.
- `getBoardView() const` → builds and returns `BoardView(gameEngine.getGameState().getBoard())` —
  a read-only snapshot for the UI to render. The UI is expected to poll this every
  frame rather than caching engine state itself; when animations are added, this same
  method is where in-motion positions would be exposed, with no change needed to the
  UI's call site.

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

`BoardView(const Board&)` now loops `row`/`col` and calls `board.getPiece(Position(row,
col))` for each cell, producing a dense, row-major 64-slot vector — empty squares get a
default `PieceView(PieceType::Empty, PieceColor::None, ...)` at that position — so it's
consistent with `BoardView::getPiece(row, col)`'s `pieces[row*cols+col]` indexing.
(Previously this constructor iterated `Board::getPieces()`, a sparse, insertion-ordered
vector of only occupied squares, which produced wrong results when combined with
`getPiece(row,col)` — fixed as part of wiring up `Controller::getBoardView()`.)

## IO layer (`src/logic/IO/`)

- `BoardParser::parse(text)` — static, parses whitespace-separated `<color><TYPE>`
  tokens (e.g. `wK`, `bQ`, `.` for empty) into a `Board`. Row width must be consistent
  or it throws. Piece IDs come from a process-global static counter (not reset per
  parse — fine for uniqueness, not reproducible across repeated parses in one run).
- `BoardPrinter::print(board)` — static, row-major space-separated `Piece::toString()`
  output, inverse of the parser's format.
- `CommandProcessor::run()` — a stdin-driven REPL: reads board text until a
  `"Commands:"` line, parses it, builds `GameState`/`GameEngine`/`Controller`, then
  dispatches commands: `click x y`, `wait <ms>`, `print board`, `jump x y`. This is
  the **only current code path that actually exercises the full logic stack
  end-to-end**, but it's console/text-only — no OpenCV involved. Its entry point
  (`src/tests/main.cpp`) is presently not attached to a CMake target (see Build
  system).

Two on-disk data formats exist but are **not read by any current C++ code**:
- `assets/pieces1/board.csv` — 8x8 CSV, `<TYPE><COLOR>` per cell (reverse order from
  `BoardParser`'s `<color><TYPE>` text format).
- `assets/pieces*/*/states/*/config.json` — `{"physics": {"speed_m_per_sec", "next_state_when_finished"}, "graphics": {"frames_per_sec", "is_loop"}}`. Looks like planned
  animation/state-machine metadata for the cooldown-state rendering that doesn't exist
  yet (no JSON library is even linked in `CMakeLists.txt`).

## UI / rendering layer (`src/UI/`)

- `Img` — RAII `cv::Mat` wrapper. `read()` loads via `cv::imread(..., IMREAD_UNCHANGED)`
  (preserves alpha). `draw_on()` alpha-blends a 4-channel image onto another at (x,y).
  `show()` opens a blocking `cv::imshow` + `cv::waitKey(0)`.
- `PixelPosition` — `{int x, y}`, pixel-space analog of `Position`.
- `CoordinateConverter` — cell→pixel via a duplicated `CELL_SIZE = 100`. **Currently
  unused** — `Renderer`/`BoardCanvas` do their own inline pixel math instead.
- `BoardCanvas` — wraps the board background `Img` + `cellSize`. `getCellPosition(row,
  col)` is yet another independent cell→pixel calculation. `drawPiece(img, row, col)`
  composites a piece sprite onto the board image.
- `SpriteManager` — builds sprite file paths as
  `{assets}/{piecesFolder}/{TYPE}{COLOR}/states/{state}/sprites/{frame}.png` and loads
  them via `Img::read` on every call (its path-string cache does not cache decoded
  images, so this doesn't save disk I/O). **`stateToString` is out of sync with the
  actual on-disk folder names**: it emits `"moving"`/`"captured"` but the folders are
  `idle/move/jump/short_rest/long_rest` — requesting a `Moving`-state sprite will fail
  to load. `typeToString`'s default case and `colorToString`'s fallback also produce
  wrong-but-plausible values for `Empty`/`None` inputs rather than erroring (latent,
  since `Renderer` filters out `Empty` pieces before calling it).
- `Renderer::render(const BoardView&)` — iterates all cells, skips `Empty`, always
  requests **`PieceState::Idle`, frame `1`** regardless of the piece's actual state —
  no animation, no cooldown visuals. Calls `canvas.show()` once at the end (a single
  blocking frame, not a game loop).

## Current state of integration — read this before adding features

The logic layer (rules, engine, real-time arbiter) and the rendering layer are
**not connected**. Concretely:

- `src/main.cpp` (the real game executable) never instantiates `GameEngine`,
  `Controller`, `RuleEngine`, `RealTimeArbiter`, or `BoardParser`. It hand-builds a
  static classic starting position as a `BoardView` and renders **one frame**, then
  the program ends after a blocking `waitKey`. There is no mouse/keyboard input
  handling, no game loop, no repeated rendering.
- The only place the full logic stack runs end-to-end is the text-only REPL
  (`CommandProcessor` / `src/tests/main.cpp`), which has no rendering and (as of this
  review) isn't attached to a CMake build target.
- Wiring these together requires at least: fixing `SpriteManager::stateToString` to
  match on-disk folder names (or extending `PieceState`), adding a real game loop with
  OpenCV input handling to `main.cpp` (mouse callback → `Controller::handlePixelClick`,
  a repeated tick calling `Controller::wait` + `Controller::getBoardView` +
  `Renderer::render`), and deciding how continuous real time should drive
  `GameEngine::advanceTime` (there's no timer/thread anywhere yet). Note also that
  `BoardCanvas::show()` currently calls a blocking `cv::waitKey(0)`, which would need
  to become non-blocking (e.g. `cv::waitKey(1)`) for a real per-frame loop.
  `Controller::handlePixelClick`/`getBoardView` (the UI-facing entry points described
  in "Controller & command flow" above) already exist and are ready to be called from
  such a loop.

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

1. Logic layer isn't wired into the graphical executable (see above) — the biggest one.
2. ~~`BoardView(const Board&)` produces a wrongly-ordered/sparse vector~~ — fixed: it
   now builds a dense, row-major vector consistent with `getPiece(row, col)`'s indexing.
3. `PieceState` enum (3 values) doesn't cover the on-disk cooldown states (`jump`,
   `short_rest`, `long_rest`) that the real-time mechanic needs for visual feedback,
   and `SpriteManager::stateToString` doesn't even match the 3 values it does have
   (`"moving"` vs. on-disk `"move"`).
4. `Piece::setState` is never called outside tests — no production code marks a piece
   as `Moving`/`Captured`.
5. `RealTimeArbiter` supports only one in-flight motion/jump globally, not per-piece —
   a deliberate MVP simplification worth knowing about before extending it.
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
| Change what happens when a move completes (capture, promotion, game-over) | `GameEngine::executeMove` |
| Change click/selection UX | `src/logic/Controller/Controller.cpp` |
| Parse/print board text | `src/logic/IO/BoardParser.cpp` / `BoardPrinter.cpp` |
| Add real GUI input + a live loop | `src/main.cpp` — currently absent, needs to be built from scratch, wiring a mouse callback → `Controller::handlePixelClick`, plus a repeated tick calling `Controller::wait` + `Controller::getBoardView` + `Renderer::render` |
| Fix/extend sprite rendering | `src/UI/SpriteManager.cpp` (state-name mismatch), `src/UI/Renderer.cpp` (hardcoded Idle/frame 1) |
| Change how pixel clicks map to board cells | `src/logic/Controller/BoardMapper.cpp`, fed via `Controller::handlePixelClick` |
| Add/adjust tests | `src/tests/logic_tests/<matching-folder>/` |
