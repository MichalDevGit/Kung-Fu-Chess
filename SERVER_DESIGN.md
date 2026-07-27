# Cloud Server Architecture Design

## System Overview

This cloud server architecture is designed to support a large-scale real-time gaming
environment: many registered users, many concurrent active players spread globally, and
continuous per-piece move/state updates for every in-progress game. The system is built
as a set of independently deployable, horizontally scalable services rather than a
single monolithic server process, so that connection handling, matchmaking, and game
simulation can each be scaled and operated independently.

**Core design principle:** neither the client nor the Gateway ever decides the rules of
the game. Move/jump requests are opinions, not commands — the `GameEngine` running
inside a Game Server Shard remains the single source of truth for game state, exactly as
it is today in the existing codebase.

## Key Architectural Components

### API Gateway

- **Function:** The entry point for all non-real-time client requests — registration,
  login, room/game lookup, and historical results. Issues the identity/session token a
  client then presents to the WebSocket Gateway.
- **Problems Solved:** Keeps request/response traffic (which doesn't need a persistent
  connection) off the WebSocket tier, and gives the system one place to enforce auth
  before a client ever reaches a live game connection.

### WebSocket Gateway

- **Function:** Accepts and holds the persistent WebSocket connection for every
  connected client, authenticates it against the token issued by the API Gateway, and
  routes each client's `move`/`jump` traffic to whichever Game Server Shard currently
  owns that client's session. Also relays the shard's outbound state pushes back down
  the same connection.
- **Problems Solved:** Decouples "how many open sockets can we hold" from "how many
  games can we simulate," so each can scale independently; lets a client reconnect
  through any gateway instance rather than being pinned to one process that also runs
  game logic.

### Matchmaker

- **Function:** A shared, gateway-agnostic queue of players waiting for a game, pairing
  entries by score proximity and queue order, backed by a store all Matchmaker instances
  can see (see Redis below), not by any one process's local memory.
- **Problems Solved:** Lets matchmaking keep working correctly no matter which gateway
  or node a given player's `find_game` request happened to land on.

### Game Allocator

- **Function:** Once the Matchmaker produces a pair, the Game Allocator decides which
  Game Server Shard will host that match (based on current shard load/capacity), creates
  the session there, and informs the WebSocket Gateway(s) serving both players where to
  route their traffic.
- **Problems Solved:** Separates "who plays whom" (Matchmaker's job) from "which physical
  server runs the simulation" (this component's job), enabling load-aware placement and
  making it possible to add/remove shard capacity without touching matchmaking logic.

### Game Server Shards

- **Function:** Each shard runs some number of independent, authoritative game sessions
  — the `GameEngine`/`Controller`/rules simulation — exactly as `GameSession` does today,
  advancing each game's real-time clock on its own tick and publishing state changes for
  the WebSocket Gateway to push to the two participants.
- **Problems Solved:** Isolates game simulation load from connection-handling load;
  a crash or restart of one shard only affects the games it was hosting, not the whole
  fleet; shards can be added or removed to match simulation demand independently of
  gateway/connection capacity.

### Observability

- **Function:** Centralized logging, metrics, and health checks across every service
  (API Gateway, WebSocket Gateway, Matchmaker, Game Allocator, Game Server Shards), plus
  load testing to validate capacity assumptions before they're needed in production.
- **Problems Solved:** Gives operators a single place to see the health and load of every
  moving part, and to catch regressions or capacity limits before players do.

## Recommended Technologies

1. **NATS / Redis Pub-Sub** — internal, low-latency communication between services (e.g.
   Matchmaker → Game Allocator match notifications, shard → gateway routing updates).
2. **Redis** — temporary, shared state: active sessions, matchmaking queue, reconnect
   windows, gateway-to-shard routing table. Replaces today's in-process-only
   `ConnectionRegistry`/`Matchmaker`/session indexes so every service instance sees the
   same state.
3. **PostgreSQL** — permanent data: user accounts, historical scores/ratings, completed
   game results, move history.
4. **Docker Compose** — running a full local instance of every service together, for
   development and integration testing.
5. **Kubernetes / K3s** — running and scaling all services in a managed way in
   production, with self-healing and rolling updates.

## Resiliency and Fault Tolerance

### Graceful Disconnections and Reconnection Windows

If a player's connection drops, the WebSocket Gateway grants a grace period (e.g., 10
seconds) before the owning Game Server Shard forfeits the game. The player can reconnect
through any WebSocket Gateway instance — not necessarily the one they started on — and,
using the shared Redis routing/session state, be reattached to the same in-progress
session on the same shard.

### Master-Replica Redis and Service Redundancy

Redis operates in a Master-Replica configuration so a primary node failure fails over to
a replica without losing matchmaking/session state. Every service (API Gateway,
WebSocket Gateway, Matchmaker, Game Allocator, Game Server Shards) runs as multiple
replicas behind Kubernetes, which replaces any crashed instance automatically; a crashed
Game Server Shard's in-flight sessions are the one exception that requires explicit
handling (see below), since simulation state currently lives only in that shard's
memory.

## Known Gaps Against the Current Codebase

This design is not yet implemented. The existing server (`KungFuChessServer`) is a
single process combining today's equivalent of the API Gateway, WebSocket Gateway,
Matchmaker, and Game Server Shard into one address space, with all matchmaking/session/
connection state held in local in-memory containers and no Redis, NATS, PostgreSQL,
Docker, or Kubernetes usage anywhere yet. In particular, a Game Server Shard crash today
loses every session it was hosting outright — there is no session snapshotting to Redis
or PostgreSQL to recover from, and that remains open work even after the service split
above is implemented.
