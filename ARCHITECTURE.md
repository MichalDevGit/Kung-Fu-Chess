# KungFuChess — Architecture Overview

This document is a standing reference for AI assistants (and humans) working on this
codebase. It describes how the project is currently built, not how it should
eventually look. Read this before exploring the source tree — it should make a full
re-read of the sources unnecessary for most tasks. If you change the architecture in a
way that makes a section below wrong, update that section in the same change.

Last reviewed: 2026-07-26, against the source tree as of the networked client/server
split (six phases, tracked in that order): (1) `src/common/` promoted to a shared
top-level `common/` library; (2) the domain logic (`src/logic/*`) moved to
`server/src/game/` as the server's authoritative simulation; (3) `protocol/` extended
with game-session messages (`join_game`/`move`/`jump`/`game_view`/`game_started`/
`game_over`) and every `common/DTO` type given its own `toJson()`/`fromJson()`;
(4) `server/src/services/GameSession` + `GameSessionManager` +
`server/src/handlers/GameRequestHandler` built to host the authoritative game and
dispatch JSON commands to it, plus a server-owned tick loop; (5) `src/UI/*` moved to
`client/src/ui/`, and a new `client/src/game/GameClient` built as the network-backed
replacement for a locally-held `Controller&`; (6) the now-empty `src/` deleted
entirely, retiring the old single-process "hotseat" executable for good. **This is now
a real client/server game**: `server/` hosts the one authoritative `GameSession`,
`client/`'s GUI (`KungFuChessGuiClient`) is a thin renderer that only ever talks to it
over `ws://` JSON messages — there is no in-process path left connecting UI to game
logic. See "Server / persistence / networking layer" below for the full design,
including a real limitation worth knowing about before extending it further: there is
no real server-to-client push yet, so a connected client's view of the game only
refreshes when *it* sends a `move`/`jump` request, not continuously.

## What this project is

"KungFuChess" is a real-time chess variant (no turns — in the classic Kung Fu Chess
rules, pieces move independently with per-piece cooldowns), implemented in **C++17**
across three cooperating processes-worth of code: `server/` (persistence, the
authoritative game engine, and the WebSocket API), `client/` (a GUI client rendered
with **OpenCV**, plus a text-based CLI client), and `protocol/`/`common/` (the shared
JSON contract and shared value types both ends link). It is an early-stage /
in-progress codebase: the domain logic (rules, move validation, real-time timing) is
fairly well developed and unit-tested; the network layer covers session join and
move/jump commands but not yet real server-push, rooms/matchmaking beyond one implicit
session, or reconnect handling.

## Build system

- Root `CMakeLists.txt` no longer globs any sources directly — every target lives in
  one of four `add_subdirectory()`s: `common/`, `protocol/`, `server/`, `client/`, each
  with its own `CMakeLists.txt`. The old local, no-network `KungFuChess`/
  `KungFuChess_tests` executables (which used to glob all of `src/`) are gone —
  `src/` itself was deleted once the networked client/server path was proven working.
- OpenCV is vendored under `lib/` (headers in `lib/include/opencv2`, prebuilt libs/DLLs
  in `lib/bin`), version 4.5.1. Debug links `opencv_world451d`, Release links
  `opencv_world451`. The root `CMakeLists.txt` only *defines* `OPENCV_DIR`/
  `OPENCV_INCLUDE_DIR`/`OPENCV_LIB_DIR`; only `client/`'s `KungFuChessClientUi` library
  actually links OpenCV now (previously two executables did) — the DLL is copied next
  to `KungFuChessGuiClient` post-build.
- C++ standard: 17.
- Root `CMakeLists.txt` `FetchContent`s `ixwebsocket` (pinned `v12.0.1`, built with
  `USE_TLS`/`USE_ZLIB` off — plain `ws://` only, no compression) and `nlohmann_json`
  (pinned `v3.12.0`, header-only) **once, at the root**, before any `add_subdirectory`,
  since `FetchContent_MakeAvailable` for the same package from two different
  subdirectories would redefine its targets. `server/CMakeLists.txt` separately
  `FetchContent`s `SQLiteCpp` (pinned `3.3.1`) itself, since only `server/` needs it.
- Target graph, by directory (`->` means "links"):
  - `common/`: `KungFuChessCommon` (STATIC — DTOs/enums/EventBus/Config have real
    `.cpp` files, not just headers) `-> nlohmann_json`.
  - `protocol/`: `KungFuChessProtocol` (INTERFACE) `-> nlohmann_json, KungFuChessCommon`
    (its `Message.h` wraps `common::GameView` directly); `KungFuChessProtocol_tests`
    (doctest, message round-trip serialization) `-> KungFuChessProtocol` only —
    deliberately not `KungFuChessGame`, see "DTO layer" below for why that separation
    matters and how it's kept true.
  - `server/`: `KungFuChessPersistence` (STATIC, SQLite) `-> SQLiteCpp` ->
    `KungFuChessAuth` (STATIC, `services/AuthService.cpp` only) `-> KungFuChessPersistence`;
    `KungFuChessGame` (STATIC, all of `src/game/` — model/rules/Engine/Controller/IO)
    `-> KungFuChessCommon`; `KungFuChessGameSession` (STATIC,
    `services/GameSession.cpp`+`GameSessionManager.cpp` — kept separate from
    `KungFuChessAuth` on purpose, see below) `-> KungFuChessGame, KungFuChessProtocol`;
    `KungFuChessNetwork` (STATIC, `handlers/`+`network/`)
    `-> KungFuChessAuth, KungFuChessGameSession, KungFuChessProtocol, ixwebsocket`;
    `KungFuChessServer` executable `-> KungFuChessNetwork`;
    `KungFuChessPersistence_tests` (doctest; persistence + auth + game + game-session +
    common tests, all in one binary) `-> KungFuChessPersistence, KungFuChessAuth,
    KungFuChessGame, KungFuChessGameSession, KungFuChessNetwork`.
  - `client/`: `KungFuChessClientNetwork` (STATIC, `network/WebSocketClient`)
    `-> KungFuChessProtocol, ixwebsocket`; `KungFuChessCliClient` executable (the
    text-based auth-only CLI) `-> KungFuChessClientNetwork, KungFuChessCommon`;
    `KungFuChessClientGame` (STATIC, `game/BoardMapper.cpp`+`GameClient.cpp`)
    `-> KungFuChessClientNetwork, KungFuChessCommon`; `KungFuChessClientUi` (STATIC,
    `ui/` — moved from `src/UI/`) `-> KungFuChessClientGame, KungFuChessCommon, OpenCV`;
    `KungFuChessGuiClient` executable (`src/gui_main.cpp` — the actual playable
    networked game) `-> KungFuChessClientUi, KungFuChessClientGame,
    KungFuChessClientNetwork, KungFuChessCommon`; `KungFuChessClient_tests` (doctest;
    `BoardMapper` + basic `GameClient` construction/default-state coverage)
    `-> KungFuChessClientGame`.

## Module map and dependency direction

```
common/                <-- no dependency on server/ or client/
  enums/                   PieceColor, PieceType, PieceState, RestKind, MoveValidationReason
  Config/                  BoardConfig (CELL_SIZE), TimingConfig (move/rest/tick durations),
                           NetworkConfig (host/port) -- plain constexpr, single source of
                           truth for values previously hardcoded/duplicated per file
  EventBus/                EventBus.h (generic pub/sub) + Events.h (chess event payloads)
  DTO/                     BoardView/PieceView/PositionView/MotionView/JumpView/RestView/
                           GameView -- each with a toJson()/fromJson() pair; each type's
                           converting-from-domain-object constructor (e.g.
                           PositionView(const Position&)) lives in its own *FromDomain.cpp
                           translation unit, deliberately separate from that type's other
                           methods (see "DTO layer" below for why)
        ^
protocol/               <-- depends on common/ (wraps common::GameView directly)
  MessageType.h            wire "type" string constants
  Message.h                one struct per message, each with in-struct toJson()/fromJson()
        ^
server/src/game/*       <-- the authoritative domain simulation, depends only on common/
  model, rules, Engine, Controller, IO   (moved verbatim from src/logic/*, see below)
        ^
server: GameSession, GameSessionManager, GameRequestHandler   <-- depends on game/ + protocol/
        ^
server/src/network (WebSocketServer)   <-- request/response transport, unaware of game/ at all

client/src/ui/*         <-- depends only on common/DTO + common/enums + client/src/game/PixelPosition
  Img, BoardCanvas, SpriteManager, AnimationFrame, Renderer, GameLoop
        ^ (GameLoop is the one exception: it also holds a GameClient&)
client/src/game/*       <-- BoardMapper (pixel math) + PixelPosition ({int x, y}) +
                            GameClient (network-backed Controller& replacement),
                            depends on common/ + protocol/
        ^
client/src/network (WebSocketClient)   <-- request/response transport, unaware of game/ at all
```

`common/` is the *only* thing `server/` and `client/` share as compiled code — and even
then, only the DTO/enum/EventBus/Config layer, never a live game object. The two
processes talk **exclusively** over serialized JSON (`protocol/`); there is no shared
`Controller&`/`GameEngine&` reference anywhere in this codebase anymore, unlike before
the client/server split when one `main.cpp` held all of it in one address space.

`server/src/game/Controller` no longer has any pixel-facing methods or a `BoardMapper`
member — pixel↔cell translation is purely a client-side rendering concern now (see
`client/src/game/BoardMapper`, whose `pixelToCell` returns a `PositionView`, not a
domain `Position`, precisely so the client never needs to link the server's domain
model at all). `Controller::click(Position)` (click-to-select, still used by the REPL/
tests) and the newer `Controller::move(Position, Position)` (a direct, unconditional
from/to command, added for the network's fully-resolved `MoveRequest`) are two
independent entry points into the same engine — see "Controller & command flow" below
for why both exist.

## Core domain model (`server/src/game/model/`)

- **`Position`** — `{int row, col}` value type. Has `operator<` so it can go in
  `std::set<Position>` (used by rule legality sets).
- **`Motion`** — an in-flight move for the real-time engine: `{bool active; Position
  from, to; long long startTime, endTime;}`. `isFinished(now)` / `hasStarted(now)`.
- **`Jump`** — same shape as `Motion` but for a stationary "jump" ability: `{bool
  active; Position position; long long startTime, endTime;}`. Represents a temporary
  ambush/counter window at a single square, not a move.
- **`Rest`** — `{int pieceId; long long startTime, endTime; RestKind kind;}`.
  Represents a per-piece "just moved/jumped, can't move/jump again yet" cooldown.
  Unlike `Motion`/`Jump`, it's keyed by **piece id**, not `Position` (a piece stays put
  while resting, so identity — not square — is what a `requestMove`/`requestJump`
  guard needs to check), and it has no `active` flag: it lives in `RealTimeArbiter`'s
  `std::map<int, Rest>`, where map presence is itself the "active" signal. `RestKind`
  (`common/enums/RestKind.h`, `Long`/`Short`) is what lets a single `Rest` per piece
  represent either a post-*move* cooldown or a post-*jump* cooldown.
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

## Rules system (`server/src/game/rules/`)

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

`PieceState` (`common/enums/PieceState.h`) has 6 values: `Idle, Moving, Captured,
Jump, LongRest, ShortRest`. This still does **not** fully match the on-disk sprite
state folders (`idle/move/jump/short_rest/long_rest`; `Captured` maps to `"captured"`,
which has **no on-disk folder at all** — deliberately unimplemented, see Gaps).

`RealTimeArbiter` (`server/src/game/Controller/RealTimeArbiter`) is a scheduler with a
manually advanced logical clock (`long long currentTime`, moved forward only by
explicit `advanceTime(ms)` calls — **no threads, no wall-clock inside this class
itself**; see "Server-owned tick loop" below for what *does* call `advanceTime` on a
real interval now). Important simplification: it tracks only **one global `Motion` and
one global `Jump`** for the entire board — not per-piece; a second
`requestMove`/`requestJump` while one is active is rejected with
`MoveAlreadyInProgress`. `Rest`, however, *is* per-piece (keyed by piece id) — multiple
pieces can be resting simultaneously; only the `Motion`/`Jump` themselves remain
single-global.

`GameEngine` (`server/src/game/Engine/GameEngine`) drives the simulation. Its
constructor takes an `EventBus&` (stored as a reference member — safe because
`GameEngine` is never actually copied, only ever constructed once via
`GameFactory`'s guaranteed-elided prvalue return), which it publishes to at the points
noted below:
- `MILLIS_PER_SQUARE`, `REST_DURATION_MILLIS`, `JUMP_REST_DURATION_MILLIS` —
  `static constexpr` members that now delegate to `common/Config/TimingConfig.h`
  (single source of truth; these were three independently-set constants before).
- `requestMove(from, to)` / `requestJump(position)` — no board mutation at request
  time; validates and starts a `Motion`/`Jump` in the arbiter.
- `executeMove(motion)` — runs when a motion *completes*: the ambush-capture special
  case (an active enemy `Jump` at the destination captures the mover instead), or an
  ordinary capture-then-move-then-promotion-check. Publishes `PieceCapturedEvent`/
  `GameOverEvent` as appropriate.
- `advanceTime(ms)` — advances the arbiter's clock, settles completed motions/jumps
  (publishing `MoveExecutedEvent` and starting the matching `Rest`), then purges
  expired rests.

## Event bus (`common/EventBus/`)

Unchanged mechanism from before the client/server split — a generic, header-only,
type-safe publish/subscribe bus (`EventBus.h`, zero dependency on anything chess-
specific) plus the chess-specific payload structs (`Events.h`: `GameStartedEvent`,
`MoveExecutedEvent{pieceId, from, to}`, `PieceCapturedEvent{pieceId, color, type,
position}`, `GameOverEvent`). **What changed**: this bus finally has a real subscriber.
`server/src/services/GameSession` constructs its own `EventBus`, passes it into its own
`GameEngine`, and subscribes a handler for all four event types (see "Server /
persistence / networking layer" below) — this is the seam where "a game state change
just happened" becomes "tell the network layer to broadcast something," with `EventBus`
as the single source of "what changed," so there's no separate diffing/polling logic
anywhere else.

## Controller & command flow

`Controller` (`server/src/game/Controller/Controller`) is the sole mediator between a
game-hosting owner (today: `GameSession`) and `GameEngine` — nothing outside `server/`
ever touches `GameEngine` directly. It holds `GameEngine&`, `hasSelection`,
`selectedPosition`. No `BoardMapper`, no pixel-facing methods at all (both moved to
`client/` — see Module map above).

- `click(Position)`: selects a same-color piece on first click (ignores clicks on
  empty squares), re-selects if a second same-color piece is clicked, otherwise calls
  `requestMove` and clears selection regardless of validation outcome. Still used
  directly by the REPL (`CommandProcessor`) and unit tests, which already work in
  logical-position space. **Not used by the network path** — `GameSession` never
  calls `click()`, only `move()`.
- `move(Position from, Position to)` — a **direct**, unconditional from/to command:
  `gameEngine.requestMove(from, to)`, bypassing `click()`'s selection state machine
  entirely. Added specifically because a networked `MoveRequest` already names both
  squares explicitly; routing it through `click()`'s selection state would risk
  misfiring against whatever this `Controller`'s selection happened to be at the time.
  This means **`Controller`'s own `hasSelection`/`selectedPosition` are never set by
  the network path at all** — the client-visible "selection highlight" is now a purely
  client-side concept (see `client/src/game/GameClient` below).
- `jump(Position)` → `requestJump`. `wait(ms)` → `advanceTime` — called by
  `GameSessionManager::tickAll`, not by any client request (see below).
  `printBoard(ostream&)` → `BoardPrinter::print`.
- `getGameView() const` → builds and returns a `GameView` snapshot exactly as before
  the split (board/motion/jump/rests/selection/currentTime) — this is what
  `GameSession` serializes into a `protocol::GameViewMessage` to send to a client.
- `isGameOver() const` → forwards to `gameEngine.getGameState().isGameOver()`.

`PixelPosition` (`client/src/game/PixelPosition`) — a plain `{int x, y}` value type.
Moved out of `common/` since only `client/` ever used it (`BoardMapper` and
`GameLoop`'s mouse-event handling); it now lives next to `BoardMapper`, its main
consumer.

## DTO layer (`common/DTO/`)

`PositionView`, `PieceView`, `BoardView`, `MotionView`, `JumpView`, `RestView`,
`GameView` mirror the domain model for rendering *and* for the wire — each now has a
`toJson() const` (returning a `nlohmann::json`, composing child DTOs' own `toJson()`s
rather than duplicating field lists) and a `static X fromJson(const nlohmann::json&)`.
Enum wire values (`PieceColor`/`PieceType`/`PieceState`/`RestKind`) are registered once
via `NLOHMANN_JSON_SERIALIZE_ENUM` in `common/enums/EnumJson.h` — one canonical string
per enum value, not re-invented per call site.

`MotionView`/`JumpView` gained a second, all-fields constructor (mirroring `RestView`'s,
which already had one) alongside their original converting-from-domain constructor —
needed so a side with no domain `Motion`/`Jump` object (the client) can still
reconstruct one purely from JSON via `fromJson`.

**Important structural detail, easy to break by accident**: each DTO's
converting-from-domain constructor (`PositionView(const Position&)`,
`PieceView(const Piece&)`, `BoardView(const Board&)`, `MotionView(const Motion&)`,
`JumpView(const Jump&)`) lives in its own dedicated `*FromDomain.cpp` file (e.g.
`PositionViewFromDomain.cpp`), **separate** from that DTO's other constructors/
`toJson()`/`fromJson()`/getters (which stay in the original `X.cpp`). This is not
cosmetic: static libraries link at object-file granularity, so a consumer that never
calls a converting-from-domain constructor (a future client always falls in this
category — it only ever builds DTOs from `fromJson`) never causes the linker to pull
that specific `.obj` out of `KungFuChessCommon.lib`, and therefore never needs
`server/src/game/model`'s symbols linked in at all. `protocol/tests`
(`KungFuChessProtocol_tests`) is proof this works today: it links only
`KungFuChessProtocol` (→ `KungFuChessCommon`), never `KungFuChessGame`, and still
builds — before this split, it needed `KungFuChessGame` linked in purely to satisfy
symbol references inside converting constructors it never actually called. **If you
ever merge a DTO's converting-from-domain constructor back into its main `.cpp` file,
you will silently re-introduce this coupling** — any consumer that used to link fine
without `KungFuChessGame` will fail at link time instead.

`common/DTO`'s *headers* still `#include` `server/src/game/model/*.h` directly (for
the converting constructors' declarations) — so the decoupling described above is a
link-time, object-file-granularity thing, not a compile-time/header-level one. A
`client/`-side translation unit including, say, `PieceView.h` still transitively sees
`server/src/game/model/Piece.h`'s declaration; it just never needs `Piece.cpp`'s
compiled symbols linked in, because nothing in that translation unit ever constructs a
`PieceView` from a real `Piece`.

## IO layer (`server/src/game/IO/`)

- `GameFactory::createNewGame(EventBus& eventBus)` — the production entry point,
  called by `GameSession`'s constructor. Takes the caller's `EventBus` and forwards it
  straight into the `GameEngine` it builds. Returns a fully-populated `GameEngine` as a
  single prvalue (guaranteed C++17 copy elision — required, since `GameEngine`'s
  `RuleEngine` member stores raw pointers to its own rule sub-objects).
  `createClassicBoard()` (private) builds the standard starting position directly.
- `BoardParser::parse(text)` / `BoardPrinter::print(board)` — REPL/testing
  convenience, not used by production game setup.
- `CommandProcessor::run()` — a stdin-driven REPL: reads board text, builds
  `GameState`/`GameEngine`/`Controller`, then dispatches `click <row> <col>` (now takes
  logical row/col directly, **not** pixels — its old `BoardMapper`-based pixel-to-cell
  translation was removed along with `BoardMapper`'s move to `client/`, since pixel
  math has no business in server-side code), `wait <ms>`, `print board`,
  `jump <row> <col>`. **Its entry point no longer exists at all** — it used to be
  `src/tests/main.cpp`, which was deleted along with the rest of `src/` once the
  networked path was proven working, so `CommandProcessor` is now compiled (as part of
  `KungFuChessGame`) but has no way to actually run outside of
  `server/tests/game_tests/io/command_processor_test.cpp`'s stdin-injection test.

## UI / rendering layer (`client/src/ui/`)

Unchanged from before the split in every respect **except what `GameLoop` talks to**
(see below) — `Img`, `BoardCanvas`, `SpriteManager`, `AnimationFrame`, `Renderer` all
still depend only on `common/DTO`/`common/enums`/`client/src/game/PixelPosition`, exactly as
before, and none of them know a network is involved. `CoordinateConverter` is still
unused (`Renderer`/`BoardCanvas` do their own inline pixel math). `BoardCanvas`'s
constructor `cellSize` argument, `SpriteManager`'s `spriteSize` argument, and
`BoardMapper`/`CoordinateConverter`'s `CELL_SIZE` all now derive from the single
`common/Config/BoardConfig::CELL_SIZE` — previously three independently-hardcoded
`100`s, now one source of truth (see Gaps history).

- `GameLoop` — the live loop and the only place in `client/` that touches input
  directly. Constructed with `GameClient&` (not `Controller&` anymore), `Renderer&`,
  `BoardCanvas&`. `run()`: registers the mouse callback once, then loops:
  `renderer.render(gameClient.getGameView())` → `renderer.renderGameOver()` if
  `gameClient.isGameOver()` → `cv::waitKey(1)` → stops on ESC. **No longer measures a
  wall-clock delta or calls anything like `wait(deltaMs)`** — the authoritative clock
  now advances on the server's own tick loop (see below), completely independent of
  whether this render loop is even running. Mouse handlers call
  `gameClient.handlePixelClick`/`handlePixelJump` — same method names as the old
  `Controller` calls, just resolving to network requests now.

## `client/src/game/` — `BoardMapper` and `GameClient`

- `BoardMapper` — moved here from `server/src/game/Controller/BoardMapper` (its
  original home, before the split). `pixelToCell(...)` returns a `common::PositionView`
  now, not a domain `Position` — a deliberate change made when it moved, since the
  client must never depend on `server/src/game/model` at all; `PositionView` is
  already the "safe for a network client" row/col carrier `common/DTO` provides.
- `GameClient` — the network-backed stand-in for what used to be a directly-held
  `Controller&`. Exposes the exact same method surface `GameLoop` always called:
  `handlePixelClick(PixelPosition)`, `handlePixelJump(PixelPosition)`, `getGameView()`,
  `isGameOver()`. Constructed with a `WebSocketClient&`; registers itself as that
  client's message handler (via the new `WebSocketClient::setOnMessage`, added
  specifically to solve this construction-order problem: `GameClient` can't exist
  before `WebSocketClient` does, so the handler can't be supplied at `WebSocketClient`
  construction time the way `CliShell`'s simpler print-only handler is) and
  immediately sends a `JoinGameRequest`.
  - `handlePixelClick` reimplements `Controller::click()`'s old select-then-move UX,
    but against the **last-received `BoardView`/`PieceView` snapshot** instead of a
    live `Board`/`Piece` — first click selects a same-color piece (checked against
    that snapshot), a second click on a different-color piece or empty square sends a
    `protocol::MoveRequest{fromRow, fromCol, toRow, toCol}` and clears the client's own
    selection. This selection state is **entirely local to `GameClient`** — the
    server-side `Controller::move()` path never sets `hasSelection` at all (see
    "Controller & command flow" above), so `getGameView()` here substitutes this
    class's own tracked `hasSelection`/`selectedPosition` into the `GameView` it
    returns, specifically so the selection-highlight rendering feature (in
    `AnimationFrame`/`Renderer`) keeps working under the network split.
  - `handlePixelJump` is a direct, stateless `protocol::JumpRequest` send — mirrors
    `Controller::jump()`'s always-direct behavior, no selection involved.
  - `onMessage(json)` (private, invoked from `WebSocketClient`'s background thread) —
    on a `game_view` message, replaces the stored `GameView` snapshot; on `game_over`,
    sets a local flag `isGameOver()` reads. Any other type, or any parse failure, is
    silently ignored (never throws out of a socket callback). All shared state is
    behind one `std::mutex`, since this callback runs on a different thread than
    `handlePixelClick`/`getGameView`/`isGameOver` (the render loop's thread) —
    same category of problem `CliShell`'s output mutex already solves, just for
    mutable game state instead of interleaved console output.

## Current state of integration

There is exactly one way to play now: start `KungFuChessServer`, then run
`KungFuChessGuiClient` against it. `gui_main.cpp` connects a `WebSocketClient`,
constructs `GameClient` around it (which joins the session), then builds
`BoardCanvas`/`SpriteManager`/`AnimationFrame`/`Renderer`/`GameLoop` exactly as the old
(now-deleted) local `src/main.cpp` did with a `Controller`. The old single-process,
no-network "hotseat" path (`GameFactory`+`Controller`+`GameLoop` all in one address
space) no longer exists in any form — it was fully retired once this networked path
was verified end-to-end.

**Known limitation, not yet addressed**: `server/src/network/WebSocketServer` is still
purely request/response (one inbound message → one reply on the same connection,
nothing unsolicited) — this was deliberately left unchanged during the client/server
split. `GameSession`'s `EventBus` subscribers *do* build the right
`protocol::GameViewMessage`/`GameStartedMessage`/`GameOverMessage` for every event that
fires, but the `BroadcastFn` callback they're handed today (wired up in
`server/main.cpp`) just logs to console — there is no real per-connection push
transport for it to call yet. In practice this means: a client's `GameView` only
refreshes as the *response* to that same client's own `move`/`jump` request (which
`GameRequestHandler` always returns fresh); a motion settling, a capture resolving, or
game-over triggered by the server's own tick loop between two client actions is
**not** visible to that client until its next action. Building real server push
(tracking connections per session, extending `WebSocketServer` to send unsolicited
messages) is the natural next step, not yet done.

What's still missing/rough, carried over unchanged from before the split:
- **Sprite-level animation** still doesn't cover `Captured` (no on-disk sprite folder
  exists for it at all — see Gaps).
- No visual feedback for check/checkmate/stalemate (none of those are implemented at
  the rules level either — see Gaps).
- Password hashing (`server/src/persistence/UserRepository`) is still plain text.
- `wss://`/TLS still doesn't exist — plain `ws://` only.

## Testing

doctest framework (single header, vendored as its own copy in each of
`server/tests/doctest.h`, `client/tests/doctest.h`, `protocol/tests/doctest.h` — kept
as separate copies rather than a shared location, since each is an independent build
target and this is a vendored single-header third-party file, not project code).

- `server/tests/` — `auth_service_test.cpp`, `user_repository_test.cpp` (persistence/
  auth, no sockets), `game_session_test.cpp`/`game_request_handler_test.cpp` (new —
  `GameSession`'s event→broadcast wiring with a fake broadcast callback, and
  `GameRequestHandler`'s JSON dispatch/error handling), `common_tests/` (moved from
  `src/tests/common_tests/` — `EventBus`/`PieceStateToString`/`TimeProgress`), and
  `game_tests/` (moved from `src/tests/logic_tests/` — mirrors
  `server/src/game/{model,rules,Engine,Controller,IO}`, same one-`TEST_CASE`-per-class/
  `SUBCASE`-per-scenario pattern, no mocking). All in one binary,
  `KungFuChessPersistence_tests` (name kept from before the split rather than renamed,
  since it now covers considerably more than persistence).
- `protocol/tests/` (new) — round-trip `toJson()`→`fromJson()` coverage for every game
  message struct, including a full `GameViewMessage` round-trip through a real
  `BoardView`/`MotionView`/`JumpView`/`RestView` snapshot.
- `client/tests/` (new) — `board_mapper_test.cpp` (moved from
  `src/tests/logic_tests/controller/`, updated for `BoardMapper`'s `PositionView`
  return type) and `game_client_test.cpp` (construction/default-state/no-crash-without-
  a-live-connection coverage only — `GameClient`'s actual click-to-select-then-move
  decision logic depends on real board content that only ever arrives from a live
  server's `GameViewMessage`, so that path is verified manually against a running
  server instead of faked in a unit test).

Notably **no tests for `client/src/ui/*`**, no tests for `CoordinateConverter`, and no
tests for the DTOs' `toJson()`/`fromJson()` round-trips *outside* of what
`protocol/tests` happens to exercise via `GameViewMessage` — worth adding direct
coverage for `BoardView`/`PieceView`/etc.'s own serialization independent of the
message wrapper.

## Server / persistence / networking layer (`server/`, `protocol/`, `client/`)

The full networked client/server design. `server/` hosts persistence, the
authoritative game engine, and both the auth and game-command JSON APIs; `protocol/`
is the shared wire-format contract; `client/` has two executables — the original
auth-only CLI, and the actual playable GUI.

### `protocol/` — shared JSON message contract

Header-only, depended on by `common/` (its `Message.h` wraps `common::GameView`),
`server/`, and `client/`, so no one duplicates the wire format.

- `MessageType.h` — the JSON envelope's `"type"` string constants: `register`,
  `login`, `register_result`, `login_result`, `error`, and (new) `join_game`,
  `game_joined`, `move`, `jump`, `game_view`, `game_started`, `game_over`.
- `Message.h` — one struct per message, each with an in-class `toJson()`/`fromJson()`
  pair (built on `nlohmann::json`) — the only place that knows that message's field
  names. Auth structs (`RegisterRequest`/`LoginRequest`/`RegisterResult`/
  `LoginResult`/`ErrorResult`) are unchanged from before the split. New game structs:
  `JoinGameRequest`/`GameJoinedResult{sessionId}`,
  `MoveRequest{fromRow, fromCol, toRow, toCol}`, `JumpRequest{row, col}`,
  `GameViewMessage{GameView view}` (wraps the DTO snapshot directly, calling its own
  `toJson()`/`fromJson()`), `GameStartedMessage`/`GameOverMessage` (empty payloads,
  only the `"type"` tag carries information — mirroring `GameStartedEvent`/
  `GameOverEvent`, which are likewise empty). `readType(json)` is unchanged.

### `server/` — persistence + game engine + WebSocket networking

- `server/src/persistence/*` — unchanged: `UserRecord`, `Database`, `UserRepository`
  (passwords still plain text, deliberately postponed).
- `server/src/services/AuthService` — unchanged: thin, networking-agnostic wrapper
  around `UserRepository`, its own mutex (shared SQLite connection, called
  concurrently once wired into `WebSocketServer`).
- `server/src/game/` — the domain simulation, moved here from `src/logic/*` verbatim
  (see Module map / Core domain model / Rules system / Real-time mechanics above for
  what actually lives here — nothing about the rules/timing/mechanics changed, only
  the location and `Controller`'s pixel-facing surface, which was removed).
- `server/src/services/GameSession` — **new**. Owns one `EventBus` + one `GameEngine`
  (built via `GameFactory::createNewGame`) + one `Controller` — this is the seam where
  "UI and logic talk in-process" became "network client and authoritative server talk
  over JSON." Publishes `GameStartedEvent` itself right after construction (the same
  contract the old `main.cpp` used to fulfill manually) and subscribes handlers for
  all four `Events.h` types: `GameStartedEvent`/`GameOverEvent` map to their matching
  protocol message; `MoveExecutedEvent`/`PieceCapturedEvent` (which have no dedicated
  wire message) instead trigger a fresh `GameViewMessage` broadcast, since that already
  conveys the resulting state fully — no need to invent a redundant per-event message
  shape. Every public method (`requestMove`/`requestJump`/`tick`/`getGameView`/
  `isGameOver`) locks its own `std::mutex` — genuinely necessary, not defensive
  filler: the server's tick-loop thread and a request-handling thread can both call
  into the same session's `GameEngine`, which has no locking of its own (same category
  of problem `AuthService`'s mutex already solves for `UserRepository`'s shared
  connection).
- `server/src/services/GameSessionManager` — **new**. Owns every active `GameSession`
  keyed by id, guarded by its own separate mutex (the map itself, not any one
  session's game state, is what needs protecting here — the tick loop iterates this
  map while a request-handling thread can concurrently insert into it via
  `getOrCreateDefaultSession`). Today only ever exposes one implicit session; this is
  the extension point for real multi-game support later (a pure addition — real
  `join`/`create`-by-id is additive on top of an already-real id→session map, not a
  redesign).
- `server/src/handlers/GameRequestHandler` — **new**, parallel to
  `AuthRequestHandler`: dispatches `join_game`/`move`/`jump` to
  `GameSessionManager`/`GameSession`, returns a fresh `protocol::GameViewMessage`
  snapshot after every `move`/`jump` (this is currently the *only* way a client's view
  actually updates — see "Current state of integration" above for the real limitation
  this implies), `ErrorResult` on anything malformed. Never throws.
- **Server-owned tick loop** (`server/src/main.cpp`) — a detached `std::thread`
  sleeping `TimingConfig::SERVER_TICK_INTERVAL_MILLIS` then calling
  `sessionManager.tickAll(...)`, forever, for the life of the process. This replaces
  the old `GameLoop`-driven wall-clock `controller.wait(deltaMs)` call — the
  authoritative clock now advances whether or not any client is even connected.
- `server/src/main.cpp` also composes `AuthRequestHandler` and `GameRequestHandler`
  behind one `dispatch()` function: tries the auth handler first, only falls back to
  the game handler if the auth handler's response is specifically
  `{"type":"error","error":"unknown_type"}` — so neither handler needs to know the
  other's message-type set; adding a new type to either one never requires touching
  this dispatch.
- `server/CMakeLists.txt` — see "Build system" above for the full target graph.
  `KungFuChessGameSession` is deliberately its own target, not folded into
  `KungFuChessAuth`, even though both live under `src/services/` — a library named
  "Auth" building `GameSession` would be a misleading name for what it contains.

### `client/` — CLI client (unchanged) + GUI client (new, the actual game)

- `client/src/network/WebSocketClient` — mostly unchanged; gained one new method,
  `setOnMessage(MessageHandler)`, letting the message handler be replaced after
  construction (needed so `GameClient`, which doesn't exist yet when `WebSocketClient`
  is first constructed, can register itself once it does).
- `client/src/cli/CliShell` + `client/src/main.cpp` — unchanged: the interactive
  auth-only CLI (`register`/`login`/`help`/`quit`), still a text-based stand-in from
  before the GUI existed.
- `client/src/ui/*` + `client/src/game/{BoardMapper,GameClient}` + `client/src/gui_main.cpp`
  — the actual playable client; see "UI / rendering layer" and "`client/src/game/`"
  above for the full breakdown, and "Current state of integration" for how they're
  wired together and run.

Not yet done: real server→client push (see "Current state of integration" above);
real multi-session join/create by id (`GameSessionManager` is ready for it, nothing
calls it yet); password hashing; `wss://`/TLS; request/response correlation IDs (fine
today since each client only ever has one request in flight at a time — `GameLoop`'s
click handling is synchronous with respect to the UI thread); reconnect/resume-session
handling if a client's connection drops mid-game.

## Known gaps / things to be careful about when editing

1. ~~Logic layer isn't wired into the graphical executable~~ — long since fixed, and
   then superseded entirely: the graphical executable itself (the old local, no-network
   `KungFuChess`) is now retired. The one graphical executable that exists,
   `KungFuChessGuiClient`, is wired into the server over the network instead.
2. ~~`BoardView(const Board&)` produces a wrongly-ordered/sparse vector~~ — fixed, and
   unaffected by the client/server split (this constructor still only ever runs
   server-side, since only the server has a real `Board` to convert from).
3. ~~`PieceState` enum doesn't cover the on-disk `short_rest` cooldown state~~ — fixed.
   `Captured` still has no backing on-disk sprite folder — latent, since no code path
   ever requests it.
4. `Piece::setState` is never called outside tests — no production code marks a piece
   as `Moving`/`Captured`.
5. `RealTimeArbiter` supports only one in-flight motion/jump globally, not per-piece —
   a deliberate MVP simplification, unaffected by the client/server split. `Rest` is
   the one part of this that *is* per-piece.
6. ~~Cell-pixel size (`100`) hardcoded independently in three places~~ — fixed as part
   of the client/server split: `common/Config/BoardConfig::CELL_SIZE` is now the single
   source of truth for `BoardMapper`, `CoordinateConverter`, and the
   `BoardCanvas`/`SpriteManager` construction calls in `gui_main.cpp`.
7. `config.json` (animation/physics metadata) and `board.csv` still exist on disk but
   are parsed by no current code, unaffected by the split.
8. No check/checkmate/castling/en-passant/stalemate — only raw movement legality plus
   king-capture-ends-game. Unaffected by the split.
9. ~~`src/tests/main.cpp` (the REPL entry point) is not attached to any current CMake
   target~~ — superseded: `src/` (including that file) was deleted entirely in the
   final phase of the client/server split. `CommandProcessor` (the REPL's underlying
   logic) still exists and is still compiled (into `KungFuChessGame`), but now has
   **no entry point of any kind** — only `command_processor_test.cpp`'s stdin
   injection exercises it. If a text-based server-side debugging tool is wanted again,
   it needs a new entry point built from scratch, not a resurrection of the old one
   (which depended on the now-removed `BoardMapper`-in-`Controller` pixel translation).
10. Dead code: a large commented-out earlier `Img::draw_on` implementation still sits
    in `client/src/ui/img.cpp`, unaffected by the split.
11. **New, from the client/server split**: no real server→client push transport yet —
    see "Current state of integration" above. This is the biggest remaining gap in the
    networked design; everything else in this list is a pre-existing, smaller issue.
12. **New**: `common/DTO`'s converting-from-domain constructors must stay in their own
    separate `*FromDomain.cpp` files, not be merged back into each DTO's main `.cpp` —
    see "DTO layer" above for exactly why, and what silently breaks if this is ignored.

## Where to look for X

| Task | Start here |
|---|---|
| Change/add a piece's movement rule | `server/src/game/rules/<Piece>Rule.cpp`, registered in `RuleEngine`'s constructor |
| Change move/jump timing (cooldowns, speed) | `common/Config/TimingConfig.h`, `server/src/game/Engine/GameEngine.cpp`, `server/src/game/Controller/RealTimeArbiter.cpp` |
| Change the post-move (`RestKind::Long`) or post-jump (`RestKind::Short`) rest/cooldown duration or its guard | `common/Config/TimingConfig.h`, `server/src/game/Engine/GameEngine.cpp` (`settleCompletedMotions`/`settleCompletedJumps`/the ambush branch of `executeMove`), `server/src/game/Controller/RealTimeArbiter.cpp` |
| Change what happens when a move completes (capture, promotion, game-over) | `server/src/game/Engine/GameEngine.cpp` (`executeMove`) |
| Change click/selection UX (server-side REPL) | `server/src/game/Controller/Controller.cpp` (`click`) |
| Change the direct from/to move command the network uses | `server/src/game/Controller/Controller.cpp` (`move`), `server/src/handlers/GameRequestHandler.cpp` |
| Change client-side click-to-select-then-move UX | `client/src/game/GameClient.cpp` (`handlePixelClick`) |
| Parse/print board text (REPL only, not production setup) | `server/src/game/IO/BoardParser.cpp` / `BoardPrinter.cpp` |
| Change the initial/starting board setup | `server/src/game/IO/GameFactory.cpp` (`createClassicBoard`) |
| Change the live loop, mouse handling, or when the game stops | `client/src/ui/GameLoop.cpp` |
| Change the per-frame data the UI renders from | `server/src/game/Controller/Controller.cpp` (`getGameView`), `common/DTO/GameView.h`/`MotionView.h`/`JumpView.h`/`RestView.h` |
| Change the traveling-piece animation, jumping-piece animation, resting-piece animation, selection/jump highlights, or rest-progress bar | `client/src/ui/AnimationFrame.cpp`, `client/src/ui/Renderer.cpp` (draw order), `client/src/ui/BoardCanvas.cpp` (pixel math, colors) |
| Fix/extend sprite rendering | `common/enums/PieceStateToString.h`, `client/src/ui/SpriteManager.cpp`, `client/src/ui/AnimationFrame.cpp` |
| Change how pixel clicks map to board cells | `client/src/game/BoardMapper.cpp`, fed via `GameClient::handlePixelClick`/`handlePixelJump` |
| Add/adjust server-side domain tests | `server/tests/game_tests/<matching-folder>/`, `server/tests/common_tests/` |
| Add a new event / subscribe to a game event | `common/EventBus/Events.h` (new event struct), publisher call sites in `server/src/game/Engine/GameEngine.cpp`, subscriber wiring in `server/src/services/GameSession.cpp` (`subscribeToEvents`) |
| Change the `users` table / user persistence | `server/src/persistence/Database.cpp` (schema), `server/src/persistence/UserRepository.cpp`; tests in `server/tests/user_repository_test.cpp` |
| Change register/login business logic (not the DB rows) | `server/src/services/AuthService.cpp`; tests in `server/tests/auth_service_test.cpp` |
| Change game-session hosting, tick behavior, or session lookup | `server/src/services/GameSession.cpp`/`GameSessionManager.cpp`; tests in `server/tests/game_session_test.cpp` |
| Change the JSON wire format for a message | `protocol/include/protocol/Message.h` (field names, `toJson`/`fromJson`), `protocol/include/protocol/MessageType.h` (the `"type"` string); round-trip tests in `protocol/tests/message_test.cpp` |
| Add a DTO's own `toJson`/`fromJson`, or change one | `common/DTO/<Type>.h`/`.cpp` — but keep any converting-from-domain constructor in `<Type>FromDomain.cpp` (see DTO layer / Gaps #12 above) |
| Add a new WebSocket message type/command | `server/src/handlers/GameRequestHandler.cpp` (or `AuthRequestHandler.cpp`) for dispatch, `protocol/include/protocol/Message.h`/`MessageType.h` for the new request/result structs, `client/src/game/GameClient.cpp` or `client/src/cli/CliShell.cpp` for the client side — `WebSocketServer`/`WebSocketClient` themselves don't need to change for a new message type |
| Change how the server binds/accepts WebSocket connections, or add real server push | `server/src/network/WebSocketServer.cpp` (the only file including ixwebsocket's server headers) — see Gaps #11 for why this is the natural next piece of work |
| Change how the GUI/CLI client connects or sends/receives | `client/src/network/WebSocketClient.cpp` (the only file including ixwebsocket's client headers) |
