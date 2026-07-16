# KungFuChess — Architecture Overview

This document is a standing reference for AI assistants (and humans) working on this
codebase. It describes how the project is currently built, not how it should
eventually look. Read this before exploring the source tree — it should make a full
re-read of `src/` unnecessary for most tasks. If you change the architecture in a way
that makes a section below wrong, update that section in the same change.

Last reviewed: 2026-07-16, against the source tree as of commit `9b1b490`, updated
same-day to reflect the new `Controller::handlePixelClick`/`getBoardView`/`isGameOver`
UI-mediation API, the `PixelPosition` move to `common/`, the `BoardView(const Board&)`
fix, and — the big one — the new `GameFactory` + `UI/GameLoop` that finally wire the
logic layer into a live, playable graphical executable.

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

`BoardView(const Board&)` now loops `row`/`col` and calls `board.getPiece(Position(row,
col))` for each cell, producing a dense, row-major 64-slot vector — empty squares get a
default `PieceView(PieceType::Empty, PieceColor::None, ...)` at that position — so it's
consistent with `BoardView::getPiece(row, col)`'s `pieces[row*cols+col]` indexing.
(Previously this constructor iterated `Board::getPieces()`, a sparse, insertion-ordered
vector of only occupied squares, which produced wrong results when combined with
`getPiece(row,col)` — fixed as part of wiring up `Controller::getBoardView()`.)

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
  `show()` is a pure drawing call — `cv::imshow(windowName(), img)` only, no
  `cv::waitKey` and no return value. Pumping the window's event queue / reading a
  keypress is deliberately **not** this class's job (see `GameLoop` below) —
  `Img`/`BoardCanvas`/`Renderer` only ever produce pixels, never interpret input.
  `windowName()` is a shared static constant (`"KungFuChess"`) so other code (namely
  `GameLoop`, for `cv::setMouseCallback`) can address the same window `show()` draws to.
- `PixelPosition` — moved to `common/PixelPosition` (see module map) — `{int x, y}`,
  pixel-space analog of `Position`.
- `CoordinateConverter` — cell→pixel via a duplicated `CELL_SIZE = 100`. **Currently
  unused** — `Renderer`/`BoardCanvas` do their own inline pixel math instead.
- `BoardCanvas` — holds a pristine `background` `Img` (loaded once from the board
  image) plus a `frame` `Img` (the actual draw buffer) and `cellSize`. `beginFrame()`
  resets `frame = background` — this exists because `drawPiece` permanently blends
  piece sprites into whatever image it's given; without resetting to a clean
  background before each frame, pieces would leave visible trails as they move across
  repeated `render()` calls in the live loop. `getCellPosition(row, col)` is yet
  another independent cell→pixel calculation. `getWindowName()` forwards to
  `Img::windowName()`.
- `SpriteManager` — builds sprite file paths as
  `{assets}/{piecesFolder}/{TYPE}{COLOR}/states/{state}/sprites/{frame}.png` and loads
  them via `Img::read` on every call (its path-string cache does not cache decoded
  images, so this doesn't save disk I/O). **`stateToString` is out of sync with the
  actual on-disk folder names**: it emits `"moving"`/`"captured"` but the folders are
  `idle/move/jump/short_rest/long_rest` — requesting a `Moving`-state sprite will fail
  to load. `typeToString`'s default case and `colorToString`'s fallback also produce
  wrong-but-plausible values for `Empty`/`None` inputs rather than erroring (latent,
  since `Renderer` filters out `Empty` pieces before calling it).
- `Renderer::render(const BoardView&)` — `canvas.beginFrame()`, then iterates all
  cells, skips `Empty`, always requests **`PieceState::Idle`, frame `1`** regardless of
  the piece's actual state — no animation, no cooldown visuals yet — then
  `canvas.show()`. Purely a drawing operation: `void`, no return value, no awareness
  that a "frame" is part of a loop or that input exists.
- `GameLoop` (new) — the live game loop and the **only** place in `UI/` that touches
  input or `logic/*`. Constructed with `Controller&`, `Renderer&`, `BoardCanvas&`.
  `run()`: registers `cv::setMouseCallback` on `canvas.getWindowName()` once (a static
  trampoline casts `void* userdata` back to `GameLoop*` and calls
  `controller.handlePixelClick(PixelPosition(x, y))` on `EVENT_LBUTTONDOWN`), then
  loops: measure wall-clock delta via `std::chrono::steady_clock` → `controller.wait(deltaMs)`
  (this is what finally drives `GameEngine::advanceTime` off a real clock, instead of
  only a REPL/test calling it manually) → `renderer.render(controller.getBoardView())`
  → `cv::waitKey(1)` (owned here, not in `Renderer`/`Img`, since interpreting a keypress
  is an input decision) → stops the loop on ESC or `controller.isGameOver()`.

## Current state of integration

The logic layer is now wired into the graphical executable:
`main.cpp` calls `GameFactory::createNewGame()` → `Controller` → constructs
`BoardCanvas`/`SpriteManager`/`Renderer` → hands them plus the `Controller` to a
`GameLoop` and calls `run()`. This is now the only entry point needed to play a game
with a live board, mouse input, and real-time piece movement.

What's still missing/rough:
- No animation, no cooldown-state visuals — `Renderer` always draws `PieceState::Idle`
  frame `1` (see gaps list). `SpriteManager::stateToString` also doesn't match the
  on-disk folder names, so requesting a non-Idle sprite would fail if anything ever
  asked for one.
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
| Parse/print board text (REPL only, not production setup) | `src/logic/IO/BoardParser.cpp` / `BoardPrinter.cpp` |
| Change the initial/starting board setup | `src/logic/IO/GameFactory.cpp` (`createClassicBoard`) |
| Change the live loop, mouse handling, or when the game stops | `src/UI/GameLoop.cpp` |
| Fix/extend sprite rendering | `src/UI/SpriteManager.cpp` (state-name mismatch), `src/UI/Renderer.cpp` (hardcoded Idle/frame 1) |
| Change how pixel clicks map to board cells | `src/logic/Controller/BoardMapper.cpp`, fed via `Controller::handlePixelClick` |
| Add/adjust tests | `src/tests/logic_tests/<matching-folder>/` |
