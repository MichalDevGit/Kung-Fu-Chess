# Cloud Migration Roadmap

This document lays out how to transition the current single-process server
(`KungFuChessServer`) toward the distributed architecture described in
[SERVER_DESIGN.md](SERVER_DESIGN.md) — API Gateway, WebSocket Gateway, Matchmaker,
Game Allocator, Game Server Shards, Observability — **incrementally**, so that at every
point along the way the system is fully working and deployable, even if the transition
stalls or is never finished.

## Guiding principle

Migrate in the **strangler-fig** pattern: extract one seam at a time, always leaving the
current monolith as the thing that actually runs in production until its replacement is
proven equivalent. Every phase below ends with a fully working two-player game and a
green test suite — nothing is "half-migrated" on `main` at any point. The rule for every
phase: extract the *interface* first (behind the same in-process call), prove it's
equivalent, *then* move it across a process boundary.

## Phase 0 — Groundwork (no architecture change, zero risk)

- Add a Dockerfile for the existing monolith exactly as it is today (single
  `KungFuChessServer`, SQLite file on a mounted volume). No new services yet — just
  proves the current server is containerizable.
- Add basic observability to the current process: structured logging (replace
  `std::cout` with leveled logs) and one health-check endpoint. Purely additive; nothing
  about game logic changes.
- Add a `docker-compose.yml` that runs today's single server + a placeholder
  Postgres/Redis container (unused by the app yet) so the compose file exists before
  anything depends on it.

**Exit criteria:** monolith runs identically inside Docker; existing test suite and
manual two-client play session still pass.

## Phase 1 — Externalize state stores, keep one process

Do this before splitting any process, since a split-brain problem (two processes
disagreeing about session state) is much harder to debug than a store swap in a single
process.

- **Users/ratings → PostgreSQL:** add `PostgresUserRepository` implementing the existing
  `IUserRepository`, add a `RepositoryBackend::Postgres` case to `RepositoryFactory`.
  Keep SQLite as the default backend until Postgres is verified in a staging run; flip
  the default only once it's proven, same interface either way, no caller changes.
- **Connection/Matchmaker/session state → Redis-backable:** introduce interfaces around
  today's `ConnectionRegistry`, `Matchmaker`'s storage, and `GameSessionManager`'s index
  maps (mirroring the `IUserRepository` pattern), with the current in-memory containers
  becoming the default "local" implementation and a Redis-backed implementation added
  alongside. Still one process, still passes today's tests unchanged, but the seam
  multi-node deployment will need later now exists and is exercised.

**Exit criteria:** same single server process, same behavior, but every piece of mutable
shared state is reachable through an interface with both a local and a
Redis/Postgres-backed implementation. This is the highest-leverage phase — everything
downstream depends on it.

## Phase 2 — Split off the API Gateway (auth/REST)

- Stand up a small, separate HTTP service that owns `register`/`login` against Postgres
  (via `AuthService`, which is already transport-agnostic) and issues a signed token.
- The existing WebSocket process stops doing password verification itself and instead
  validates the token on connect. Roll this out with both paths accepted for one release
  (old WS-native login + new token) so a half-deployed client/server pair during the
  transition still works, then remove the old path once every client is updated.

**Exit criteria:** login is a separate deployable service; the WebSocket process no
longer touches passwords directly; game itself is untouched and still fully playable.

## Phase 3 — Split the WebSocket Gateway from the Game Node (still one shard)

This is the biggest structural step, so it's scoped to **exactly one Game Node
instance** — no sharding logic yet, just a network hop where there used to be a function
call.

- Move `GameSessionManager`, `Matchmaker`, `GameSession`'s tick loop into their own
  process ("Game Node"), talking to the WebSocket Gateway process over NATS/Redis
  pub-sub instead of in-process calls.
- The WebSocket Gateway becomes a thin relay: holds connections, forwards
  `move`/`jump`/`find_game` to the Game Node, relays pushes back — using the
  Redis-backed connection/session state from Phase 1 so a gateway restart doesn't
  require a full reconnect storm.
- Because there's still only one Game Node, this phase is provably behavior-equivalent
  to today: same games, same rules, same reconnect story, just across a socket instead
  of a function call. Ship it, watch it in production for a while before Phase 4.

**Exit criteria:** two independently deployable processes (Gateway, Game Node), single
Game Node instance, full test coverage on the new message-passing boundary, game
behavior identical to the monolith.

## Phase 4 — Horizontal scale: Game Allocator + multiple shards

Only start this once Phase 3 has run stably, since it's the phase that actually needs
the Matchmaker's shared-queue behavior from Phase 1 to be trustworthy under concurrent
writers. Today's `GameNodeConfig::REQUESTS_CHANNEL`/`PUSHES_CHANNEL` are global pub/sub
broadcasts that only worked because Phase 3 ran exactly one Game Node — the moment a
second shard exists, every shard would receive every `move`/`jump`/`reconnect_check`
unless something routes each request to the shard that actually owns it. That routing
problem, not scaling per se, is Phase 4's real first problem, so it's split into
sub-phases the same way every prior phase extracted an interface before splitting a
process:

### Phase 4a — Shard-routing interfaces, still exactly one shard (done)

- `IGameShardRoutingStore` (`Local`/`Redis`, mirrors `ISessionIndexStore`):
  `sessionId`/`userId` -> `shardId`.
- `IShardLoadStore` (`Local`/`Redis`): each shard's live session count, so shard-picking
  can be "least loaded" instead of a blind round-robin once more than one shard exists.
- `GameAllocator`: given a `Matchmaker::Match`, picks a shard via `IShardLoadStore`,
  creates the session through an *injected* callback (not a direct
  `GameSessionManager&`, specifically so Phase 4b can turn that callback into a real
  cross-process call without `GameAllocator` itself changing shape), and records the
  decision in `IGameShardRoutingStore`.
- `gamenode/src/main.cpp` reads `KUNGFUCHESS_SHARD_ID` (default `"shard-1"`, not yet
  mandatory) and registers it before the tick loop starts; `runTickLoop`'s `onMatched`
  now goes through `GameAllocator::allocate` instead of calling
  `sessionManager.createSession` directly.
- Exactly one shard is ever registered in this sub-phase, so it's provably
  behavior-equivalent to Phase 3 — same equivalence gate every prior phase used.
  **Deliberately deferred to 4b:** nothing decrements a shard's load or unbinds routing
  when a session finishes yet (harmless with one shard; see ARCHITECTURE.md Known gaps
  #22), and the Gateway process/wire protocol are completely untouched.

### Phase 4b — Split `gamenode/` into Allocator + Shard processes (done)

- New top-level `gameallocator/` module (same precedent as `apigateway/`): owns
  `Matchmaker`, `MatchmakingRequestHandler`, `GameAllocator`, and its own matchmaking
  tick loop. This is the one true singleton in the topology.
- `gamenode/` shrinks to `GameSessionManager` + `GameRequestHandler` + a
  session-ticking-only loop, reading `KUNGFUCHESS_SHARD_ID` (default `"shard-1"`) to
  pick its own request channel (`GameNodeConfig::shardRequestsChannel(shardId)`,
  replacing the single global `REQUESTS_CHANNEL`) and to register itself with
  `RedisShardLoadStore`.
- Gateway side: new `GatewayGameRouter` (replacing `RemoteGameNodeHandler`) parses a
  request's `"type"` — `find_game` goes to `GameNodeConfig::
  MATCHMAKING_REQUESTS_CHANNEL` (the one Allocator); `move`/`jump` resolve the target
  shard via `ISessionIndexStore` (connection → session) + `IGameShardRoutingStore`
  (session → shard) and publish to that shard's specific channel;
  `connection_closed` fans out to both the Allocator's channel and the resolved shard's,
  if any. `RemoteReconnectResolver` resolves userId's shard the same way and skips the
  Redis round trip entirely if there's no routing entry (already "no active session").
- The one design wrinkle not anticipated when this phase was scoped: `GameAllocator`
  (Phase 4a) used to create a session in-process and read back a real `GameSession&` for
  its id/initial `GameView`. Once the Allocator and the shard that hosts a session are
  different processes, that's no longer available synchronously. Resolved without a
  round trip: `GameAllocator` assigns the session id itself and fires a fire-and-forget
  `GameNodeCreateSessionRequest` at the picked shard, and computes the initial
  `GameView` locally — safe because `GameFactory::createClassicBoard` assigns fixed
  piece ids on every call and a fresh engine's clock always starts at 0, so the
  starting position is provably identical no matter which process builds it.
  `GameSessionManager::createSession` now takes the session id as a parameter instead
  of generating its own, so the Allocator and whichever shard actually hosts the
  session agree on it without needing to ask each other.
- `docker-compose.yml`/`Dockerfile`: added a `gameallocator` service/`runtime-gameallocator`
  stage; renamed the `gamenode` service to `gamenode-1` (`KUNGFUCHESS_SHARD_ID: shard-1`)
  — the first of what would become `gamenode-2`/`gamenode-3` etc. Actually running more
  than one shard concurrently (the exit criteria below) is deliberately left to a
  follow-up change: Compose scaling can't inject distinct per-replica env vars cleanly,
  so scaling out means adding more named services, not flipping a flag.

### Phase 4c — Crash recovery: forfeit-on-crash (done)

- `IShardLoadStore` gained `heartbeat`/`isAlive`/`forget`: a Game Server Shard refreshes
  its own heartbeat from its tick loop every `GameNodeConfig::
  SHARD_HEARTBEAT_INTERVAL_MILLIS` (2s); `RedisShardLoadStore` backs this with a real
  TTL key (`SHARD_HEARTBEAT_TTL_MILLIS`, 6s) so a crashed shard's own key simply
  expires with nothing having to notice the crash happen.
- New `ShardHealthMonitor`, run from the Game Allocator's tick loop on the same
  cadence: for any shard whose heartbeat has lapsed, enumerates every session it was
  hosting (`IGameShardRoutingStore` gained `sessionsForShard`/`usersForSession` for
  this), resolves each participant's *current* connection (never one cached at match
  time, since a reconnect could have superseded it), and pushes
  `GameOverMessage{"shard_unavailable", 0}` — deliberately no rating change, since a
  crash is nobody's fault. The dead shard is then forgotten so it's never picked again.
- Adjacent fix that surfaced while implementing this: a session finishing *normally*
  wasn't releasing its shard-routing/load bookkeeping either, which would have let
  `ShardHealthMonitor` misforfeit a stale entry for an already-finished game.
  `GameSessionManager` gained a defaulted (no-op) `SessionFinishedFn` callback,
  wired by `gamenode/` to clean up both stores on every normal session end too.
- Snapshot-resume (`ISessionSnapshotStore`, `GameSession` state serialized on each
  `EventBus` mutation) remains a legitimate, larger follow-up — not done here, since
  "either [forfeit or resume] is fine as long as it's a defined, tested behavior rather
  than silence" (see exit criteria below).

**Exit criteria (met):** multiple Game Server Shards running concurrently, Game Allocator
load-balancing between them, a killed shard's games either fail gracefully (forfeit) or
resume from a snapshot — either is fine as long as it's a defined, tested behavior
rather than silence. Verified live against the real `docker-compose` stack (`gamenode-1`
+ `gamenode-2`, distinct `KUNGFUCHESS_SHARD_ID`s): two concurrent matches landed on the
two different shards (`GameAllocator`'s least-loaded picking), `docker kill` on the
container hosting one of them delivered a `game_over{reason: "shard_unavailable"}` to
both its players within the heartbeat TTL window while the other match — on the
surviving shard — kept ticking and accepting moves throughout, completely unaffected.

## Phase 5 — Full orchestration & observability

- Kubernetes manifests (replacing/complementing Docker Compose) for every service, with
  rolling updates and self-healing.
- Redis Master-Replica, Postgres backups, metrics + log aggregation + alerting across
  all five services (Gateway, WS Gateway, Matchmaker, Game Allocator, Game Server
  Shards).
- Load testing to validate the scale claims before they're needed for real traffic.

## How "always shippable" is enforced across phases

- Every phase is its own PR/set of PRs; `main` only ever contains a phase once its exit
  criteria are met and the existing test suite + a manual two-client play session both
  pass.
- Interfaces are extracted *before* processes are split (Phase 1 before Phase 3), so a
  stalled or abandoned later phase never leaves the system in a state where an interface
  exists but nothing implements it correctly — the in-memory/local implementation is
  always a legitimate fallback, not scaffolding to be deleted.
- Dual-path rollouts (Phase 2's token change) rather than atomic cutovers wherever a
  client/server version mismatch could otherwise break existing users.
