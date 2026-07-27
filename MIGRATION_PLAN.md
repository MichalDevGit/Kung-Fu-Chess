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
writers.

- Add the Game Allocator: on a Matchmaker pairing, it picks a shard (by load) and
  creates the session there, publishing the routing decision (which shard owns which
  session) to Redis.
- Scale Game Node → multiple Game Server Shard replicas. Gateway instances look up
  routing via Redis instead of assuming "the one Game Node."
- Add session snapshotting (Redis or Postgres) so a shard crash is recoverable instead
  of silently dropping every game it hosted — call this out explicitly since it's a real
  behavior change from today (currently a crash loses in-flight games outright).

**Exit criteria:** multiple Game Server Shards running concurrently, Game Allocator
load-balancing between them, a killed shard's games either fail gracefully (forfeit) or
resume from a snapshot — either is fine as long as it's a defined, tested behavior
rather than silence.

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
