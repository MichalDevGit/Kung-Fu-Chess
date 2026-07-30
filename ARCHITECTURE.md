# KungFuChess — Architecture Overview

This document is a standing reference for AI assistants (and humans) working on this
codebase. It describes how the project is currently built, not how it should
eventually look. Read this before exploring the source tree — it should make a full
re-read of the sources unnecessary for most tasks. If you change the architecture in a
way that makes a section below wrong, update that section in the same change.

Last reviewed: 2026-07-29, against the source tree as of MIGRATION_PLAN.md's
Phase 4c ("Crash recovery: forfeit-on-crash"), the third sub-phase of
Phase 4, layered on top of Phase 4b described below. That change: a Game
Server Shard now refreshes a Redis-backed liveness heartbeat
(`IShardLoadStore::heartbeat`, new alongside `registerShard`/`isAlive`/
`forget`; `RedisShardLoadStore` backs it with a real TTL key
-- `GameNodeConfig::SHARD_HEARTBEAT_TTL_MILLIS`, 6s -- so a crashed shard's
own key simply expires with nothing having to notice the crash happen;
`LocalShardLoadStore`'s version is a documented no-op, since there's no real
cross-process failure to simulate within one process) from `gamenode/`'s own
tick loop, on a coarser cadence than session-ticking itself
(`GameNodeConfig::SHARD_HEARTBEAT_INTERVAL_MILLIS`, 2s). The Game Allocator
runs a new `ShardHealthMonitor` (`server/src/services/Sharding/
ShardHealthMonitor.h`/`.cpp`) on the same cadence: for any registered shard
whose heartbeat has lapsed, it enumerates every session that shard was
hosting (two new `IGameShardRoutingStore` methods, `sessionsForShard`/
`usersForSession` -- Redis-backed via two more keys, `shard_sessions:
{shardId}` and `session_users:{sessionId}`, described in
`RedisGameShardRoutingStore`'s own class comment), resolves each
participant's *current* connection via `ConnectionRegistry::
findConnectionForUser` (never a connectionId cached at match time, since a
reconnect could have superseded it since), and pushes a `GameOverMessage`
with `reason = "shard_unavailable"` and no winner -- deliberately no rating
change at all, since a shard crash is nobody's fault, unlike a genuine
disconnect-timeout forfeit. The dead shard is then `forget()`-ten
(`IShardLoadStore`) so `GameAllocator::pickShard` never picks it again and
the same check doesn't keep re-firing for it. While implementing this, a
real, adjacent gap surfaced and got fixed in the same change (see Known gaps
#22, updated): a session that finishes *normally* (king capture, or an
ordinary disconnect-timeout forfeit) wasn't releasing its shard-routing/load
bookkeeping either, which would have let `ShardHealthMonitor` misforfeit a
long-finished game's stale routing entry. Fixed via a new, defaulted-to-no-op
`GameSessionManager::SessionFinishedFn` constructor parameter (so no existing
caller/test needed updating), invoked from `removeFinishedSessions` with the
finishing session's id and both participants' userIds (a new small
`GameSession::userIds()` accessor, safe to read lock-free since userId is
fixed at construction) -- `gamenode/src/main.cpp` wires this to
`shardRoutingStore.unbindSession`/`shardLoadStore.decrementLoad`.
Snapshot-resume (an `ISessionSnapshotStore`, restoring rather than forfeiting
a crashed shard's games) remains an explicitly deferred, larger follow-up,
not done here — see MIGRATION_PLAN.md's Phase 4c section and "Known gaps"
below for what's still open (the Allocator itself is still a single point of
failure; a forfeited player currently gets no ELO adjustment either way,
which is the intended behavior, not an oversight).

Previously reviewed against the source tree as of MIGRATION_PLAN.md's
Phase 4b ("Split gamenode/ into Allocator + Shard processes"), the second
sub-phase of Phase 4 ("Horizontal scale: Game Allocator + multiple shards"),
layered on top of Phase 4a described below. That change: a new top-level
`gameallocator/` module (same precedent as `apigateway/`/`gamenode/`: reuse
`server/`'s libraries directly, one new `main.cpp`, no relocated/duplicated
logic) becomes the one true singleton in the topology, owning `Matchmaker`,
`MatchmakingRequestHandler`, and `GameAllocator` -- all three moved out of
`gamenode/`, which now owns only `GameSessionManager`/`GameRequestHandler`/
the tick loop for whichever one shard it is. `GameNodeConfig::
REQUESTS_CHANNEL` (Phase 3's single, unscoped channel, which only worked
because exactly one Game Node ever ran) is gone, replaced by
`GameNodeConfig::MATCHMAKING_REQUESTS_CHANNEL` (one well-known name, since
there's still exactly one Allocator) and `GameNodeConfig::
shardRequestsChannel(shardId)` (one channel per registered shard). The
Gateway (`server/src/main.cpp`) is the thing that now decides which channel
a given request goes to -- new `GatewayGameRouter` (replacing Phase 3's
`RemoteGameNodeHandler`, which forwarded everything unconditionally) parses
just enough of a request's `"type"` to route `find_game` to the Allocator
and `move`/`jump` to whichever shard `IGameShardRoutingStore` (via the
session `RedisSessionIndexStore` already resolves the connection to) says
currently hosts it, and reads both stores read-only for exactly this;
`connection_closed` now fans out to *both* the Allocator (in case the
connection was only ever queued) and the resolved shard (if any) via
`GatewayGameRouter::notifyConnectionClosed`. `RemoteReconnectResolver`
similarly resolves userId's shard via `IGameShardRoutingStore::
findShardForUser` first and skips the Redis round trip entirely if there's
no entry (already "no active session," no need to ask anyone).
`GameNodeRequestRouter` (the shard side) dropped its `MatchmakingRequestHandler`/
`Matchmaker` dependencies and the try-matchmaking-then-game dispatch Phase 3
needed (a shard's channel never receives `find_game` at all now); new
`GameAllocatorRequestRouter` is the Allocator-side mirror, handling
`client_message`+`connection_closed` for `Matchmaker` only. The trickiest
part of this split: `GameAllocator::allocate` (Phase 4a) used to return a
real, freshly-created `GameSession&` by calling straight into a same-process
`GameSessionManager`; now the Allocator and the shard that will host the
session are different processes, so session creation became genuinely
fire-and-forget (`GameNodeCreateSessionRequest`, a new envelope in
`GameNodeBridge/GameNodeMessages.h`, published to the picked shard's channel
and never waited on) instead of a cross-process round trip -- made safe by
two facts verified against the actual code: `GameFactory::createClassicBoard`
assigns fixed piece ids (`0, 1, 2, ...`) on every call and a fresh
`GameEngine`/`RealTimeArbiter`'s clock always starts at `0`, so a freshly
matched session's initial `GameView` is **always identical** no matter which
process computes it. `GameAllocator::allocate` therefore builds that initial
view itself (a throwaway `EventBus`+`GameEngine`+`Controller`, no
`GameSession`, so no push side effects) and returns an `Allocation{sessionId,
initialView}` instead of a live session reference; `sessionId` itself is now
assigned by `GameAllocator` (an incrementing counter, same shape
`GameSessionManager` used to own before this phase) and handed to the shard
as a given, not invented there --
`GameSessionManager::createSession` takes `sessionId` as its first parameter
now for exactly this reason (a small, mechanical signature change touching
`GameSessionManager.h`/`.cpp` and every test that called it directly).
`MatchmakingRequestHandler`'s "already_in_game" guard, which used to call
`GameSessionManager::findSessionByConnection` directly, now takes
`ISessionIndexStore&` instead (a read-only lookup against the same
Redis-backed store every shard's `GameSessionManager` already writes to,
since this handler no longer shares a process with any `GameSessionManager`
at all). `docker-compose.yml` gained a `gameallocator` service (its own
health-check port, `NetworkConfig::GAME_ALLOCATOR_HEALTH_CHECK_PORT` = 9006)
and renamed `gamenode` to `gamenode-1` (`KUNGFUCHESS_SHARD_ID: shard-1`) --
the first of what would become `gamenode-2`/`gamenode-3` etc. if Phase 4b's
exit criteria (actually running more than one shard concurrently) is pursued
next, deliberately not done as part of this change (see "Known gaps" below);
`Dockerfile` gained a `runtime-gameallocator` stage, built from the same
shared build stage that now also builds `KungFuChessGameAllocator`.

Previously reviewed against the source tree as of MIGRATION_PLAN.md's
Phase 4a ("shard-routing interfaces, still exactly one shard"), the first
sub-phase of Phase 4 ("Horizontal scale: Game Allocator + multiple shards"),
layered on top of Phase 3 described below. That change: a new
`server/src/services/Sharding/` (compiled into `KungFuChessNetwork`, same
place `GameNodeBridge` lives, for the same "only gamenode/'s own wiring
needs this" reason) adds the two interfaces a real multi-shard deployment
needs before it can exist at all: `IGameShardRoutingStore`
(`LocalGameShardRoutingStore`/`RedisGameShardRoutingStore` — which shard
hosts a given `sessionId`/`userId`, the thing a Gateway or another Game Node
process would need to look up once more than one shard exists) and
`IShardLoadStore` (`LocalShardLoadStore`/`RedisShardLoadStore` — how many
live sessions each known shard is currently hosting, keyed by shard id in
one Redis Hash so `knownShards()` is a single `HKEYS`, not a keyspace scan) —
same Local-default/Redis-alternative dual-implementation pattern every
store since `IUserRepository` has used. `GameAllocator` (`server/src/
services/Sharding/GameAllocator.h`/`.cpp`) is the new seam itself: on a
`Matchmaker::Match`, it picks the least-loaded registered shard, creates the
session via an *injected* `CreateSessionFn` callback (deliberately not a
`GameSessionManager&` — see its class comment: this is what lets 4b turn
that callback into a real cross-process "publish a create-session request"
without `GameAllocator` itself changing shape at all), records the
`sessionId`/both `userId`s → `shardId` decision in
`IGameShardRoutingStore`, and bumps that shard's load.
`gamenode/src/main.cpp`'s `runTickLoop` now calls `allocator.allocate(match)`
where it used to call `sessionManager.createSession(white, black)` directly;
`main()` reads an optional `KUNGFUCHESS_SHARD_ID` (default `"shard-1"`,
**not** mandatory the way `KUNGFUCHESS_REDIS_URL` is — a single-shard
deployment works fine with every replica defaulting to the same id, since
there's nothing to disambiguate yet) and registers it with a
`RedisShardLoadStore` at startup, before the tick thread that can call
`allocate` ever starts. Because exactly one shard is ever registered today,
`GameAllocator::allocate` always resolves to it — this sub-phase is
provably behavior-equivalent to Phase 3, same as every prior phase's
interface-first discipline. **Deliberately not yet done** (left for Phase
4b, once a Game Server Shard's session-end signal has to cross a process
boundary anyway): nothing calls `IShardLoadStore::decrementLoad` or
`IGameShardRoutingStore::unbindSession` when a session finishes
(`GameSessionManager::removeFinishedSessions` doesn't know either store
exists) — harmless with one shard (load never influences which shard gets
picked when there's only one), but real bookkeeping debt once a second
shard exists; see "Known gaps" below. Nothing about the Gateway process,
`GameNodeConfig`'s channel names, or the Gateway↔Game Node wire protocol
changed in this sub-phase — that's Phase 4b's job, once there's a second
real shard to route *to*.

Previously reviewed against the source tree as of MIGRATION_PLAN.md's
Phase 3 ("Split the WebSocket Gateway from the Game Node (still one shard)"),
layered on top of Phase 2 described below. That change: `GameSessionManager`,
`Matchmaker`, `MatchmakingRequestHandler`, `GameRequestHandler`, and the
server-owned tick loop all moved into a new top-level `gamenode/` module
(mirroring `apigateway/`'s precedent — reuse a `server/` library directly
rather than relocate or duplicate its logic; `KungFuChessGameNode`'s one new
file, `gamenode/src/main.cpp`, links `KungFuChessNetwork`/`KungFuChessCommon`
straight from `server/`, exactly as `apigateway/` already links
`KungFuChessAuth`). What used to be `KungFuChessServer` is renamed
`KungFuChessWsGateway` (same `server/src/main.cpp` file, same `server/`
directory — only the executable target/binary name changed) and is now a
thin relay: it still holds every WebSocket connection and still verifies a
`login_token` (`AuthRequestHandler`, essentially unchanged), but no longer
touches `GameSessionManager`/`Matchmaker` in-process at all. The two processes
talk exclusively over two Redis pub/sub channels (`GameNodeConfig::
REQUESTS_CHANNEL`/`PUSHES_CHANNEL`, new `common/Config/GameNodeConfig.h`),
implemented with the already-vendored redis-plus-plus (no new dependency —
NATS was the plan's other option, deliberately not taken for exactly this
reason) via four new small classes under new `server/src/services/
GameNodeBridge/`: `GameNodeRequestPublisher`/`GameNodePushRouter` (the
Gateway's ends) and `GameNodePushPublisher`/`GameNodeRequestRouter` (the Game
Node's ends) — see "Server / persistence / networking layer" below for each
one's exact responsibilities. `find_game`/`move`/`jump` forwarding is pure
fire-and-forget (`RemoteGameNodeHandler`, replacing
`MatchmakingRequestHandler`/`GameRequestHandler` in the Gateway's own
`dispatch()`): verified by reading `GameClient.cpp`/`CliShell.cpp` that
neither the client's move/jump path nor `waitForMatch` ever depended on a
*synchronous* reply to those — both already only ever reacted to a later,
unsolicited push, even in the monolith, so nothing client-visible changes by
making that hop asynchronous too. The one place this phase keeps something
resembling a blocking RPC is login's reconnect check
(`AuthRequestHandler::completeLogin`'s "does this user already have an active
session" question), because `CliShell` relies on the resume push arriving
*before* `login_result` — this is now behind a new `IReconnectResolver`
interface (`LocalReconnectResolver`, a direct in-process call — what
`gamenode/`'s own request router and `auth_request_handler_test.cpp` both
use; `RemoteReconnectResolver`, a real Redis round trip with a timeout —
what the Gateway actually uses), the same "extract the interface, Local by
default, Redis for the real split" pattern `IConnectionStore`/
`IMatchQueueStore`/`ISessionIndexStore` already established in Phase 1b.
`AuthRequestHandler` itself now depends on `IReconnectResolver&` instead of a
raw `GameSessionManager&` — a behavior-preserving change verified by
`auth_request_handler_test.cpp` needing only a constructor-signature/include
update, not new assertions. `KUNGFUCHESS_REDIS_URL` — an explicit opt-in ever
since Phase 1b — is now **mandatory** for both `KungFuChessWsGateway` and
`KungFuChessGameNode` (both fail fast with a clear log line if it's unset):
a real process split can't tolerate either falling back to its own local/
in-memory `ConnectionRegistry`/`Matchmaker`/session-index storage, since the
whole scheme depends on both processes reading and writing the *same*
Redis-backed state (the Gateway's `ConnectionRegistry` write on login must be
visible to `MatchmakingRequestHandler`'s `connectionRegistry.find()` call
inside `gamenode/`, or `find_game` breaks outright) — the same class of
problem Phase 2 flagged for `KUNGFUCHESS_POSTGRES_URL`, just for Redis now
that an actual process split exists. `docker-compose.yml` gained a `gamenode`
service (own health-check port, `NetworkConfig::GAME_NODE_HEALTH_CHECK_PORT`
= 9005) and renamed `server` to `wsgateway`; `Dockerfile` gained a third
named runtime stage (`runtime-gamenode`, alongside the renamed
`runtime-wsgateway`), sharing one build stage that now also builds
`KungFuChessGameNode`. See "Known gaps" below for the one accepted race this
phase's design flags (a reconnect racing a Redis-unreachable Game Node) and
the reconnect-check's timeout-as-"no session" fallback.

Previously reviewed against the source tree as of MIGRATION_PLAN.md's
Phase 2 ("Split off the API Gateway (auth/REST)"), layered on top of Phase 1b
described below. That change, in two steps (both landed): **step A**
(dual-accept) added a new top-level `apigateway/` module — mirroring
`common/`/`protocol/`/`server/`/`client/` as its own `add_subdirectory` —
whose one executable, `KungFuChessApiGateway`, owns `register`/`login` over
plain HTTP (`apigateway/src/network/ApiGatewayServer`, a small
`ix::HttpServer` wrapper mirroring `HealthCheckServer`'s pImpl shape) by
reusing `AuthService`/`RepositoryFactory` verbatim from `server/`
(`apigateway/` links `KungFuChessAuth` directly; no auth logic is
duplicated). A successful REST `/login` now also issues a signed token via
new `common/Security/TokenService` (`common/Security/Sha256.h`/`.cpp` — a
small vendored, dependency-free HMAC-SHA256 implementation, same
vendor-rather-than-FetchContent spirit as `bcrypt_vendor`; deliberately not
a real JWT, just `base64url(payload).base64url(hmac)`, since nothing outside
this codebase ever needs to consume one). `server/`'s `AuthRequestHandler`
gained a new `login_token` branch (protocol: `MessageType::LoginWithToken`/
`LoginWithTokenRequest` in `protocol/Message.h`) that verifies this token via
`TokenService` and looks the user up by id via `IUserRepository::findById`
for username/score — no password touched — while, in this step, still also
keeping its original password-based `register`/`login` branches for a
transitional dual-accept window. **Step B** (landed in the same pass here,
though MIGRATION_PLAN.md frames it as "one release later") then deleted
those password branches and `AuthRequestHandler`'s `AuthService&` dependency
entirely, replacing it with `IUserRepository&` — the concrete, link-graph-
visible proof of "the WebSocket process no longer touches passwords
directly": `server/CMakeLists.txt`'s `KungFuChessNetwork` no longer links
`KungFuChessAuth` at all (linked directly instead: `KungFuChessPersistence`,
which it needed anyway for `IUserRepository`), so `KungFuChessServer` no
longer pulls in `KungFuChessAuth`/`KungFuChessSecurity`/`bcrypt_vendor` at
link time — verified by inspecting the actual build output, not just by
reading the CMake file. `protocol::LoginResult` gained one new optional
field, `token` (empty unless the API Gateway's REST `/login` populated it);
`protocol::RegisterRequest`/`RegisterResult`/`LoginRequest`/`LoginResult`
themselves are unchanged in shape and are now reused as-is for the
gateway's REST request/response *bodies*, not just WebSocket envelopes.
Client-side: new `client/src/network/ApiGatewayClient` (wraps
`ix::HttpClient`, the one file that does) replaces `CliShell`'s old
WS-message-based `register`/`login`, both now synchronous REST calls; a
successful `login` then sends `LoginWithTokenRequest{token}` over the
existing `WebSocketClient` connection and blocks for `login_result` exactly
as before (that part still genuinely belongs on the socket — it's what
binds connection identity and triggers a reconnect-resume check). A load-
bearing operational consequence, not a code one: `KungFuChessApiGateway`
(writes users) and `KungFuChessServer` (reads them by id) are now separate
processes that **must** agree on one user store — `docker-compose.yml` sets
`KUNGFUCHESS_POSTGRES_URL` on both services now (previously unset/optional),
which is the point Postgres stops being merely "reachable" and becomes
actually required for correctness; both also share one
`KUNGFUCHESS_TOKEN_SECRET`. See "Known gaps" below for what's still rough
about the token secret's default. `Dockerfile` gained a second named runtime
stage (`runtime-apigateway`, alongside the renamed `runtime-server`) sharing
one build stage that now also builds `KungFuChessApiGateway`.

Previously reviewed against the source tree as of MIGRATION_PLAN.md's
Phase 1b ("Connection/Matchmaker/session state → Redis-backable"), layered on
top of Phase 1a ("users/ratings → PostgreSQL") described below. That change:
`ConnectionRegistry`, `Matchmaker`, and `GameSessionManager`'s two lookup
indexes each moved their raw storage behind a small interface, mirroring the
`IUserRepository` pattern, while keeping every existing public method and
call site (`AuthRequestHandler`/`MatchmakingRequestHandler`/
`GameRequestHandler`/`server/main.cpp`) untouched — only how these three
objects get constructed in `main.cpp` changed. Each business-logic class kept
its own business logic and its own mutex (the "supersede a stale connection"
dance in `ConnectionRegistry`, the score/queue-order pairing scan in
`Matchmaker::tick`, the finished-session index cleanup in
`GameSessionManager::removeFinishedSessions`); only the maps/vector
underneath moved: `IConnectionStore` (`LocalConnectionStore`/
`RedisConnectionStore`), `IMatchQueueStore` (`LocalMatchQueueStore`/
`RedisMatchQueueStore`), `ISessionIndexStore` (`LocalSessionIndexStore`/
`RedisSessionIndexStore`) — see "Server / persistence / networking layer"
below for each interface's exact shape and why. `GameSessionManager`'s
`sessions` map of live `GameSession` objects itself stays in-process only,
unaffected — a `GameEngine` can't be externalized without a much bigger
redesign reserved for a later phase; only the two *lookup indexes*
(`connectionId`/`userId` -> `sessionId`) are Redis-backable here.
Redis client: `hiredis` + `redis-plus-plus`, both `FetchContent`-vendored
unconditionally in `server/CMakeLists.txt` (confirmed buildable from source
under MSVC, unlike libpq — no `find_package`/optional-guard dance needed
here) with one documented workaround: redis-plus-plus's own CMake hardcodes
an *installed*-style `<include>/hiredis/hiredis.h` header layout when
sniffing hiredis's version at configure time, which a bare FetchContent
build tree doesn't have (hiredis's own build points consumers straight at
its flat source dir) — `server/CMakeLists.txt` stages a copy of hiredis's
public headers into that expected shape at configure time to bridge this;
linking still goes through the real `hiredis::hiredis` target, unaffected.
Like `KUNGFUCHESS_POSTGRES_URL`, `KUNGFUCHESS_REDIS_URL` is an explicit,
unset-by-default opt-in (`server/src/main.cpp`) that switches all three
stores to Redis at once, sharing one connection; local/in-memory storage
remains the actual default until a staging run proves Redis out.

Previously reviewed against the source tree as of MIGRATION_PLAN.md's
Phase 1a ("users/ratings → PostgreSQL"), layered on top of Phase 0
("Groundwork") described below. That change: `server/src/persistence/
PostgresDatabase` (owns a `pqxx::connection` + schema, mirroring `Database`'s
role for SQLite) and `PostgresUserRepository` (implements `IUserRepository`
against it — `AuthService`/`RatingService`/`GameSessionManager` need zero
changes, same as adding `InMemoryUserRepository` before it) plus a
`RepositoryBackend::Postgres` case in `RepositoryFactory`. This backend is
**optional at configure time**: unlike SQLiteCpp/bcrypt, libpqxx links
against libpq (the official Postgres C client), whose own build can't
realistically be vendored-and-built-from-source under MSVC, so `server/
CMakeLists.txt` does `find_package(PostgreSQL QUIET)` — only if found does it
`FetchContent` libpqxx, compile `PostgresDatabase`/`PostgresUserRepository`
for real, and define `KUNGFUCHESS_HAS_POSTGRES` (both files' entire contents
are guarded by `#ifdef KUNGFUCHESS_HAS_POSTGRES`, compiling to empty
translation units otherwise); `RepositoryFactory::createUserRepository`
throws a clear error if `RepositoryBackend::Postgres` is requested in a build
where it wasn't compiled in. The Dockerfile's build stage now installs
`libpq-dev` (so this is always true there) and its runtime stage installs
`libpq5`; a native Windows configure with no system Postgres install is
completely unaffected. `PostgresUserRepository` locks its own
`std::mutex` around every method — a real difference from
`SqliteUserRepository`, needed because a single `pqxx::connection` (unlike
SQLiteCpp) isn't safe for concurrent use from multiple threads, and
`IUserRepository` is called from both connection-handling threads
(`AuthService`) and the tick thread (`RatingService`) with no shared lock
between them. **SQLite remains the actual default** — `server/src/main.cpp`
still calls `RepositoryFactory::createUserRepository(RepositoryBackend::
Sqlite, dbPath)` unconditionally unless the `KUNGFUCHESS_POSTGRES_URL`
environment variable is set, in which case it opts into
`RepositoryBackend::Postgres` with that connection string instead — an
explicit, opt-in seam for a staging run to exercise the new backend, per
MIGRATION_PLAN.md Phase 1's "keep SQLite as the default backend until
Postgres is verified in a staging run." `docker-compose.yml`'s `postgres`
service is unchanged in shape, just documented as reachable this way now
instead of merely a placeholder.

Previously reviewed against the source tree as of MIGRATION_PLAN.md's
Phase 0 ("Groundwork") — containerizing today's monolith with no architecture
change, layered on top of the user-persistence repository-pattern refactor
described below. That change: a Dockerfile (multi-stage, Linux, builds only
`KungFuChessServer`) plus a `docker-compose.yml` (the server, plus placeholder
`postgres`/`redis` containers the app doesn't talk to yet — reserved for
Phase 1). Since `client/` links vendored Windows/MSVC-only OpenCV binaries
(see "Build system" below) that can't configure or link on Linux at all, the
root `CMakeLists.txt` gained a `BUILD_CLIENT` option (default `ON`, unchanged
for every existing native/Windows workflow) guarding `add_subdirectory(client)`
— the Docker build passes `-DBUILD_CLIENT=OFF` and never touches `client/`/`lib/`
at all. Two small additions round out "basic observability": `common/Logging/
Logger` (`common/Logging/Logger.h`/`.cpp` — a minimal leveled, timestamped,
thread-safe logger; INFO/DEBUG to stdout, WARN/ERROR to stderr) replaces the
one `std::cout` startup line in `server/src/main.cpp` and the previously-silent
`catch`es in `server/src/network/WebSocketServer.cpp`'s `broadcast`/`sendTo`;
and `server/src/network/HealthCheckServer` (new, `server/src/network/
HealthCheckServer.h`/`.cpp`, wrapping `ix::HttpServer` — already vendored via
the existing ixwebsocket dependency, no new library needed) serves a plain
`GET`-anything → `200 {"status":"ok"}` liveness endpoint on its own port
(`NetworkConfig::HEALTH_CHECK_PORT`, `9003`), independent of the WebSocket/JSON
game protocol, for a container `HEALTHCHECK`/orchestrator probe to poll.
One real (if narrow) behavior change was required to make "runs identically
inside Docker" actually mean something: `WebSocketServer`'s bind address used
to be hardcoded to `"127.0.0.1"` inside `WebSocketServer.cpp` regardless of
`NetworkConfig::DEFAULT_HOST` — harmless on a native run, but fatal inside a
container, since a process bound only to loopback is unreachable through a
published Docker port no matter what. `WebSocketServer`'s (and the new
`HealthCheckServer`'s) constructor now takes a `host` parameter (defaulting to
`"127.0.0.1"`, preserving today's native behavior exactly), and
`server/src/main.cpp` reads an optional `KUNGFUCHESS_HOST` environment
variable to override it — the Dockerfile sets `KUNGFUCHESS_HOST=0.0.0.0` so
the containerized server actually binds every interface; nothing changes for
a plain local build where that variable is unset.

Previously reviewed against the source tree as of the user-persistence
repository-pattern refactor, layered on top of the bcrypt/ELO change described below.
That refactor: `server/src/persistence/UserRepository` (the single concrete class every
consumer named directly) was replaced with `IUserRepository` (a pure abstract interface —
`createUser`/`findByUsername`/`findById`/`setScore`, see `server/src/persistence/
IUserRepository.h`) plus two implementations of it: `SqliteUserRepository` (the renamed
former `UserRepository`, now owning its own `Database` internally instead of taking one by
reference, so it's the only class that still knows SQLite exists at all) and
`InMemoryUserRepository` (new — a dependency-free `std::unordered_map`-backed double,
added specifically to prove the interface is real and not just theater, and to replace
the `Database(":memory:")`-backed `TestUserRepository` boilerplate three test files used
to duplicate). `server/src/persistence/RepositoryFactory::createUserRepository(
RepositoryBackend, sqliteDbPath)` is the one place that knows both backends exist and
constructs whichever one is asked for — `server/main.cpp` calls it once with
`RepositoryBackend::Sqlite`, and that's the only place "which backend" is decided.
`AuthService`, `RatingService`, and `GameSessionManager` now all depend on
`IUserRepository&`, never a concrete class; `AuthService::registerUser` used to catch
`SQLite::Exception` directly to detect a duplicate username (a real SQLite leak into
business logic) and now catches a new backend-agnostic `DuplicateUsernameException`
(`server/src/persistence/UserRepositoryExceptions.h`) that every implementation throws
instead. See "Server / persistence / networking layer" below for the current shape of
this layer.

Previously reviewed against the source tree as of the bcrypt-hashed-passwords +
ELO-rating change, layered on top of the auth-gated matchmaking change described below.
That earlier change: (a) `server/src/security/PasswordHasher` wraps a vendored bcrypt
implementation (`bcrypt_vendor`, source-fetched via `FetchContent_Populate` in
`server/CMakeLists.txt` since its own build system targets GCC/make, not MSVC) behind a
two-method `hash`/`verify` interface, with `AuthService` now the only thing that calls
it — `UserRepository` itself no longer knows hashing exists at all, just stores whatever
credential string it's given; (b) `common/Config/RatingConfig.h` +
`server/src/services/Rating/EloCalculator` (pure math, no DB) + `server/src/services/
Rating/RatingService` (reads both players' current ratings, computes new ones, persists both)
replace the old flat `MatchmakingConfig::SCORE_DELTA_WIN`/`_LOSS`, and — fixing a real
gap this surfaced — an ordinary king-capture win now updates rating too, not just a
disconnect-timeout forfeit: `GameSession`'s `ScoreUpdateFn` became `GameOutcomeFn`
(winner/loser user ids, not a raw delta), and `GameOverEvent` gained a `loserColor` field
(read from the captured king *before* `GameEngine::executeMove` removes it from the
board — the removal used to happen first) so `GameSession` can resolve winner/loser and
call the same outcome callback both endings now share.
See "Server / persistence / networking layer" below for the full design of both.

Previously reviewed against the source tree as of the auth-gated matchmaking
change (five phases, tracked in that order, layered on top of the earlier networked
client/server split): (1) `server/src/network/WebSocketServer` threaded a stable
per-connection id through every request, plus a targeted `sendTo(connectionId, json)`
and a `setCloseHandler` for drop notifications, and `server/src/services/
Connection/ConnectionRegistry` was added to bind `connectionId -> {userId, username, score}` the
moment `login` succeeds; (2) `server/src/services/Matchmaking/Matchmaker` (a plain score/queue-order
pairing scan, driven off the existing tick thread) plus new `protocol/` messages
(`find_game`/`match_found`/`no_match`) and `server/src/handlers/
MatchmakingRequestHandler` were added, dispatched in a three-way `auth -> matchmaking ->
game` chain (see `server/main.cpp`); (3) `GameSessionManager`/`GameSession` were reworked
from "one implicit, lazily-created default session" into real per-match sessions —
`GameSessionManager::createSession` is only ever called from a `Matchmaker` pairing, each
`GameSession` now knows its two participants' user ids/usernames/connection ids and
rejects a `move`/`jump` whose requesting connection doesn't own the piece's color
(`Controller::pieceColorAt` + `GameSession::requestMove`/`requestJump`, returning a
`CommandOutcome` instead of applying blindly) — `join_game`/`game_joined` are gone
entirely, since a session now only ever comes into being via a match, which already
hands the client its `GameView`; (4) disconnect/reconnect resilience — a closed
connection marks its `GameSession` slot disconnected (pushing `opponent_disconnected` to
the other side) without pausing the tick loop, a forfeit-with-score-delta fires if
`MatchmakingConfig::RECONNECT_GRACE_MILLIS` elapses with no reconnect, and a fresh
`login` for a user with a still-active session rebinds it and pushes a
`match_found`-shaped resume instead of going through matchmaking again; (5) the client
was collapsed from two executables into one — `client/src/gui_main.cpp`/
`KungFuChessGuiClient` are retired, `client/src/main.cpp` (built as `KungFuChessClient`)
now runs the CLI auth phase, then matchmaking, then the OpenCV game pane, all against the
same connection. **This is now a real, authenticated, matched 1v1 game**: two different
users, each running their own `KungFuChessClient` process, register/log in, get
automatically paired by score/queue-order (white = whoever logged in first), and play a
color-ownership-enforced, disconnect-resilient game against each other. See "Server /
persistence / networking layer" below for the full design.

## What this project is

"KungFuChess" is a real-time chess variant (no turns — in the classic Kung Fu Chess
rules, pieces move independently with per-piece cooldowns), implemented in **C++17**
across three cooperating processes-worth of code: `server/` (persistence, matchmaking,
the authoritative game engine, and the WebSocket API), `client/` (one executable: CLI
auth, then an OpenCV-rendered game pane, over one connection), and `protocol/`/`common/`
(the shared JSON contract and shared value types both ends link). It is an early-stage /
in-progress codebase: the domain logic (rules, move validation, real-time timing) is
fairly well developed and unit-tested; the network layer now covers real per-connection
push, matchmaking, per-match session isolation, color-ownership enforcement, and
disconnect/reconnect handling; passwords are bcrypt-hashed and both game-ending paths
apply an ELO rating update — see the Gaps list below for what's still deliberately out
of scope (TLS, session persistence across a server restart, spectators, resign/draw).

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
  actually links OpenCV now — the DLL is copied next to `KungFuChessClient` post-build.
  These binaries are prebuilt for MSVC/Windows only, so `client/` cannot configure or
  link on Linux at all — the root `CMakeLists.txt`'s `BUILD_CLIENT` option (default
  `ON`) guards `add_subdirectory(client)` specifically so a server-only Linux build
  (the Docker image; see top of this document) can pass `-DBUILD_CLIENT=OFF` and skip
  `client/` entirely instead of failing partway through.
- C++ standard: 17.
- Root `CMakeLists.txt` `FetchContent`s `ixwebsocket` (pinned `v12.0.1`, built with
  `USE_TLS`/`USE_ZLIB` off — plain `ws://` only, no compression) and `nlohmann_json`
  (pinned `v3.12.0`, header-only) **once, at the root**, before any `add_subdirectory`,
  since `FetchContent_MakeAvailable` for the same package from two different
  subdirectories would redefine its targets. `server/CMakeLists.txt` separately
  `FetchContent`s `SQLiteCpp` (pinned `3.3.1`) itself, since only `server/` needs it.
  It also does `find_package(PostgreSQL QUIET)` and, only if that succeeds, `FetchContent`s
  `libpqxx` (pinned `7.9.2`) — the one dependency in this project that isn't
  vendor-buildable from source under MSVC the way SQLiteCpp/bcrypt are (libpq's own build
  needs autoconf/make/bison/flex), so it's optional rather than required; see "Server /
  persistence / networking layer" below for what's conditional on it. It also
  unconditionally `FetchContent`s `hiredis` (pinned `v1.4.0`) and `redis-plus-plus`
  (pinned `1.3.15`) — confirmed buildable from source under both MSVC and GCC (unlike
  libpq), so no optional/`QUIET` treatment is needed the way libpqxx needs. redis-plus-plus's
  own CMake calls `find_package(hiredis QUIET)` internally to locate hiredis;
  `server/cmake/Findhiredis.cmake` (a hand-written Module-mode shim, prepended onto
  `CMAKE_MODULE_PATH`) redirects that to the hiredis this project already fetched/built
  itself — deliberately not `FetchContent`'s own `OVERRIDE_FIND_PACKAGE` for this, since
  that needs CMake >= 3.24 and the Docker/Linux build's apt-installed cmake (Ubuntu
  22.04) is only 3.22; Module-mode resolution has no such version floor. See the Phase
  1b review note above for the header-layout workaround this shim also depends on.
- Target graph, by directory (`->` means "links"):
  - `common/`: `KungFuChessCommon` (STATIC — DTOs/enums/EventBus/Config have real
    `.cpp` files, not just headers) `-> nlohmann_json`.
  - `protocol/`: `KungFuChessProtocol` (INTERFACE) `-> nlohmann_json, KungFuChessCommon`
    (its `Message.h` wraps `common::GameView` directly); `KungFuChessProtocol_tests`
    (doctest, message round-trip serialization) `-> KungFuChessProtocol` only —
    deliberately not `KungFuChessGame`, see "DTO layer" below for why that separation
    matters and how it's kept true.
  - `server/`: `KungFuChessPersistence` (STATIC — `IUserRepository`/`SqliteUserRepository`/
    `InMemoryUserRepository`/`PostgresUserRepository`/`RepositoryFactory`; only
    `SqliteUserRepository.cpp`/`PostgresDatabase.cpp`/`PostgresUserRepository.cpp` actually
    touch a real database) `-> SQLiteCpp` (`-> pqxx` too, only when `PostgreSQL_FOUND`) ->
    `KungFuChessAuth` (STATIC, `services/Auth/AuthService.cpp` only) `-> KungFuChessPersistence`;
    `KungFuChessGame` (STATIC, all of `src/game/` — model/rules/Engine/Controller/IO)
    `-> KungFuChessCommon`; `KungFuChessGameSession` (STATIC,
    `services/GameSession/GameSession.cpp`+`GameSessionManager.cpp`+`services/SessionIndex/LocalSessionIndexStore.cpp`+
    `services/SessionIndex/RedisSessionIndexStore.cpp` — kept separate from
    `KungFuChessAuth` on purpose, see below) `-> KungFuChessGame, KungFuChessProtocol,
    KungFuChessPersistence, redis++_static` (`KungFuChessPersistence` added for
    `IUserRepository`, which `GameSessionManager` now takes directly to build every
    session's forfeit-score callback; `redis++_static` for `RedisSessionIndexStore` —
    see "Server / persistence / networking layer" below);
    `KungFuChessNetwork` (STATIC, `handlers/`+`network/`+`services/Connection/ConnectionRegistry.cpp`+
    `services/Matchmaking/Matchmaker.cpp`+`services/Connection/LocalConnectionStore.cpp`+
    `services/Connection/RedisConnectionStore.cpp`+`services/Matchmaking/LocalMatchQueueStore.cpp`+
    `services/Matchmaking/RedisMatchQueueStore.cpp`+`services/GameNodeBridge/*.cpp` (Phase 3;
    Phase 4b added `GatewayGameRouter`/`GameAllocatorRequestRouter` here and removed
    `RemoteGameNodeHandler` — see below)+`services/Sharding/*.cpp` (Phase 4a: `GameAllocator` +
    `IGameShardRoutingStore`/`IShardLoadStore`'s Local/Redis implementations — see below)
    — the `ConnectionRegistry`/`Matchmaker` family
    compiled here since none of it depends on `KungFuChessGame`/`KungFuChessGameSession`
    at all, and the handlers that need them already live in this library; `handlers/
    MatchmakingRequestHandler.cpp` takes `ISessionIndexStore&` instead of
    `GameSessionManager&` as of Phase 4b, for the same "this runs in a process with
    no live GameSessionManager anymore" reason)
    `-> KungFuChessPersistence, KungFuChessGameSession, KungFuChessProtocol, ixwebsocket, redis++_static`
    — **as of Phase 2 step B, deliberately does NOT link `KungFuChessAuth`** (only
    `AuthRequestHandler`'s now-removed password branches ever needed it; it now
    depends on `KungFuChessPersistence` directly instead, for `IUserRepository`,
    which it needed anyway); `KungFuChessWsGateway` executable (**Phase 3: renamed
    from `KungFuChessServer`**, same `src/main.cpp`) `-> KungFuChessNetwork`
    (so, transitively, `KungFuChessAuth`/`KungFuChessSecurity`/`bcrypt_vendor` are
    **not** linked into this executable at all anymore — see "Server / persistence /
    networking layer" below);
    `KungFuChessPersistence_tests` (doctest; persistence + auth + game + game-session +
    matchmaker + common tests, all in one binary) `-> KungFuChessPersistence,
    KungFuChessAuth, KungFuChessGame, KungFuChessGameSession, KungFuChessNetwork`
    (this test binary still links `KungFuChessAuth` directly, since `auth_service_test.cpp`
    still exercises it — only the production `KungFuChessWsGateway` executable dropped it).
  - `apigateway/` (**Phase 2**): `KungFuChessApiGatewayLib` (STATIC,
    `ApiGatewayRequestHandler.cpp`+`network/ApiGatewayServer.cpp`)
    `-> KungFuChessAuth, KungFuChessCommon, KungFuChessProtocol, ixwebsocket` — the one
    place `AuthService`/`PasswordHasher`/bcrypt are exercised at runtime outside tests;
    `KungFuChessApiGateway` executable (`src/main.cpp`) `-> KungFuChessApiGatewayLib`;
    `KungFuChessApiGateway_tests` (doctest) `-> KungFuChessApiGatewayLib, KungFuChessPersistence`
    (for `InMemoryUserRepository`). Added to the root `CMakeLists.txt` right after
    `server/` (needs `KungFuChessAuth`, defined there), unconditionally (no
    `BUILD_CLIENT`-style guard — no OpenCV dependency).
  - `gameallocator/` (**new, Phase 4b**): no library of its own, just one file —
    `KungFuChessGameAllocator` executable (`src/main.cpp`) `-> KungFuChessNetwork,
    KungFuChessCommon` — same link set as `gamenode/`/`apigateway/`, since this is
    genuinely the same code (`Matchmaker`/`MatchmakingRequestHandler`/`GameAllocator`,
    all still compiled inside `KungFuChessNetwork` under `server/`), just constructed
    by a `main()` that talks to `GameAllocatorRequestRouter`/`GameNodeRequestPublisher`
    instead of holding a `GameSessionManager` directly. Exactly one replica of this
    executable ever runs. Added to the root `CMakeLists.txt` right after `server/`
    (needs `KungFuChessNetwork`), unconditionally — no OpenCV dependency.
  - `gamenode/` (Phase 3; narrowed by Phase 4b): no library of its own, just one file —
    `KungFuChessGameNode` executable (`src/main.cpp`) `-> KungFuChessNetwork,
    KungFuChessCommon` — same link set as before, since `GameSessionManager`/
    `GameRequestHandler` are still compiled inside `KungFuChessNetwork`/
    `KungFuChessGameSession` under `server/`, just constructed by a `main()` that talks
    to `GameNodeRequestRouter`/`GameNodePushPublisher` instead of a `WebSocketServer` it
    doesn't hold. Phase 4b removed `Matchmaker`/`MatchmakingRequestHandler`/
    `GameAllocator` from this executable's own construction (moved to `gameallocator/`
    above) — this process now only ever hosts one shard's sessions. Added to the root
    `CMakeLists.txt` right after `server/` (needs `KungFuChessNetwork`),
    unconditionally — no OpenCV dependency, same as `apigateway/`.
  - `client/`: `KungFuChessClientNetwork` (STATIC, `network/WebSocketClient` +
    `network/ApiGatewayClient` — **new, Phase 2**, wraps `ix::HttpClient` for the
    REST `register`/`login` calls, the one file that does)
    `-> KungFuChessProtocol, ixwebsocket`; `KungFuChessCli` (STATIC, `cli/CliShell.cpp` —
    a library now, not its own executable, since `main.cpp` needs it plus the UI/game
    libraries below in one process) `-> KungFuChessClientNetwork, KungFuChessProtocol`;
    `KungFuChessClientGame` (STATIC, `game/BoardMapper.cpp`+`GameClient.cpp`)
    `-> KungFuChessClientNetwork, KungFuChessCommon`; `KungFuChessClientUi` (STATIC,
    `ui/` — moved from `src/UI/`) `-> KungFuChessClientGame, KungFuChessCommon, OpenCV`;
    `KungFuChessClient` executable (`src/main.cpp` — the one real client: CLI auth phase,
    then matchmaking, then the game) `-> KungFuChessCli, KungFuChessClientUi,
    KungFuChessClientGame, KungFuChessClientNetwork, KungFuChessCommon`;
    `KungFuChessClient_tests` (doctest; `BoardMapper` + `GameClient` construction/
    default-state coverage) `-> KungFuChessClientGame`.

## Module map and dependency direction

```
common/                <-- no dependency on server/ or client/
  enums/                   PieceColor, PieceType, PieceState, RestKind, MoveValidationReason
  Config/                  BoardConfig (CELL_SIZE), TimingConfig (move/rest/tick durations),
                           NetworkConfig (host/port), MatchmakingConfig (queue wait/score
                           range, reconnect grace, forfeit score deltas) -- plain constexpr,
                           single source of truth for values previously hardcoded/duplicated
  EventBus/                EventBus.h (generic pub/sub) + Events.h (chess event payloads)
  DTO/                     BoardView/PieceView/PositionView/MotionView/JumpView/RestView/
                           GameView -- each with a toJson()/fromJson() pair; each type's
                           converting-from-domain-object constructor (e.g.
                           PositionView(const Position&)) lives in its own *FromDomain.cpp
                           translation unit, deliberately separate from that type's other
                           methods (see "DTO layer" below for why)
  MonotonicClock.h         nowMillis() -- wall-clock ms (steady_clock), used only for
                           matchmaking queue timestamps and disconnect-grace timing, never
                           for the game engine's own manually-advanced logical clock
  WallClock.h              wallClockMillis() -- REAL (system_clock/Unix-epoch) ms, a
                           deliberately different clock source from MonotonicClock.h's
                           nowMillis(): steady_clock's epoch is only meaningful within
                           one process, but TokenService issues a token in one process
                           (apigateway/) and verifies it in another (server/), so it
                           needs a clock both agree on -- see Security/ below
  Security/                TokenService (issue/verify a signed login token) + Sha256
                           (vendored, dependency-free HMAC-SHA256) -- new in Phase 2,
                           shared by server/ (verifies) and apigateway/ (issues)
        ^
protocol/               <-- depends on common/ (wraps common::GameView directly)
  MessageType.h            wire "type" string constants
  Message.h                one struct per message, each with in-struct toJson()/fromJson()
        ^
server/src/game/*       <-- the authoritative domain simulation, depends only on common/
  model, rules, Engine, Controller, IO   (moved verbatim from src/logic/*, see below)
        ^
server: GameSession, GameSessionManager   <-- depends on game/ + protocol/ + persistence/
        ^ (IUserRepository, for forfeit score deltas)
server: Matchmaker, MatchmakingRequestHandler   <-- depends on persistence/ +
        services/Connection (ConnectionRegistry) + services/SessionIndex
        (ISessionIndexStore, Phase 4b: for the "already_in_game" guard, replacing
        a direct GameSessionManager& dependency -- this class runs in gameallocator/'s
        own process now, never in the same process as any GameSessionManager)
        (still compiled here, same as before -- but now only ever *constructed*
        by gameallocator/src/main.cpp, never by gamenode/ or server/src/main.cpp)
        ^
server: GameRequestHandler   <-- depends on GameSession/GameSessionManager above +
        common/Security. Constructed only by gamenode/src/main.cpp -- Phase 4b split
        this from MatchmakingRequestHandler above, which used to live in the same
        process (gamenode/, Phase 3) but now doesn't.
        ^
server: services/GameNodeBridge/*   <-- Phase 3, reshaped by Phase 4b: the Redis
        pub/sub seams between the Gateway, the Game Allocator, and a Game Server
        Shard. GameNodeRequestPublisher/GameNodePushRouter/GatewayGameRouter (the
        WebSocket Gateway's ends -- GatewayGameRouter replaced Phase 3's
        RemoteGameNodeHandler, since the Gateway now decides *which* channel a
        request goes to instead of forwarding everything to one place) +
        GameAllocatorRequestRouter (the Game Allocator's end, new in Phase 4b) +
        GameNodeRequestRouter/GameNodePushPublisher (a Game Server Shard's ends) +
        IReconnectResolver (LocalReconnectResolver/RemoteReconnectResolver, the
        latter now resolving a shard via services/Sharding's
        IGameShardRoutingStore below before ever publishing anything) -- depends
        on protocol/ + common/Config/GameNodeConfig.h + services/Sharding (for the
        two routers/resolvers that need routing/load lookups)
        ^
server: services/Sharding/*   <-- Phase 4a, reshaped by Phase 4b: GameAllocator +
        IGameShardRoutingStore/IShardLoadStore (Local/Redis). GameAllocator depends
        only on GameSession's *types* (Player) and common/DTO/GameView -- never
        GameSessionManager directly: session creation is an injected, fire-and-forget
        RequestSessionCreationFn callback (Phase 4b: a real cross-process publish to
        the picked shard's channel; Phase 4a: an in-process call, back when Allocator
        and shard were the same process), and the initial GameView is computed
        locally (GameFactory::createClassicBoard's fixed piece ids + a fresh
        RealTimeArbiter's zero clock make it deterministic) rather than read back
        from whatever the shard creates. Used only from gameallocator/src/main.cpp's
        runTickLoop now (moved out of gamenode/ in Phase 4b). Phase 4c added
        ShardHealthMonitor here too: polls IShardLoadStore::isAlive for every known
        shard (also new in 4c, alongside heartbeat/forget) and, for a dead one,
        forfeits every session IGameShardRoutingStore::sessionsForShard/
        usersForSession (also new in 4c) says it was hosting -- depends on
        protocol/ (GameOverMessage) but nothing from persistence/, deliberately
        (see its own class comment for why no rating change happens here).
        ^
server: AuthRequestHandler   <-- depends on persistence/ + common/Security + the
        IReconnectResolver& above (Phase 3: no longer a raw GameSessionManager&)
        ^
server/src/network (WebSocketServer)   <-- per-connection-id request/response + targeted
                                            push transport, unaware of game/ at all

gameallocator/src/main.cpp   <-- new, Phase 4b: constructs Matchmaker/
  (new, Phase 4b)                MatchmakingRequestHandler/GameAllocator/the
                                 matchmaking tick loop (all from server/, unchanged)
                                 plus GameAllocatorRequestRouter/GameNodeRequestPublisher/
                                 GameNodePushPublisher (from server/'s GameNodeBridge) --
                                 holds no client connection of its own, only Redis. The
                                 one true singleton in the topology -- exactly one
                                 replica of this process ever runs.

gamenode/src/main.cpp   <-- Phase 3, narrowed by Phase 4b: constructs
  (Phase 3;                  GameSessionManager/GameRequestHandler/the (session-only)
   narrowed Phase 4b)        tick loop (all from server/, unchanged) plus
                             GameNodeRequestRouter/GameNodePushPublisher (from
                             server/'s GameNodeBridge) -- holds no client connection
                             of its own, only Redis. Reads KUNGFUCHESS_SHARD_ID
                             (default "shard-1") and registers it with
                             RedisShardLoadStore at startup; subscribes to
                             GameNodeConfig::shardRequestsChannel(shardId), not a
                             single shared channel. Phase 4b removed this process's
                             own Matchmaker/MatchmakingRequestHandler/GameAllocator/
                             ConnectionRegistry construction entirely (moved to
                             gameallocator/ above) -- this process only ever hosts
                             one shard's sessions now.

apigateway/src/*        <-- depends on server/src/services/Auth/AuthService (reused directly,
  (Phase 2)                  not duplicated) + common/Security/TokenService + protocol/
  ApiGatewayRequestHandler   REST body <-> AuthService/TokenService translation, testable
                             without a real HTTP server (mirrors AuthRequestHandler's split)
  network/ApiGatewayServer  thin ix::HttpServer wrapper (mirrors HealthCheckServer)

client/src/ui/*         <-- depends only on common/DTO + common/enums + client/src/game/PixelPosition
  Img, BoardCanvas, SpriteManager, AnimationFrame, Renderer, GameLoop
        ^ (GameLoop is the one exception: it also holds a GameClient&)
client/src/game/*       <-- BoardMapper (pixel math) + PixelPosition ({int x, y}) +
                            GameClient (network-backed Controller& replacement),
                            depends on common/ + protocol/
        ^
client/src/cli/CliShell <-- the auth-phase message handler (register/login/help/quit);
                            depends on protocol/ + client/src/network only. Phase 2:
                            register/login are REST calls via ApiGatewayClient; a
                            successful login then sends the token over WebSocketClient
        ^
client/src/network (WebSocketClient, ApiGatewayClient)   <-- request/response transport,
                            unaware of game/ at all (ApiGatewayClient is new, Phase 2)
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
`server/src/services/GameSession/GameSession` constructs its own `EventBus`, passes it into its own
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
  `isGameOver()`. Constructed with a `WebSocketClient&` and the match's initial
  `GameView` (from `MatchFoundResult` — there is no more `join_game` round trip);
  registers itself as that client's message handler (via `WebSocketClient::
  setOnMessage`, added specifically to solve this construction-order problem:
  `GameClient` can't exist before `WebSocketClient` does, so the handler can't be
  supplied at `WebSocketClient` construction time), replacing whichever handler
  `CliShell`/`waitForMatch` had installed for the earlier phases.
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
  - Constructed with the match's initial `GameView` directly (from `MatchFoundResult` --
    there is no more `join_game` round trip to wait on; see "Current state of
    integration" below) instead of starting from an empty board.
  - `onMessage(json)` (private, invoked from `WebSocketClient`'s background thread,
    installed by this constructor -- replacing whatever handler `CliShell` had
    installed for the auth/matchmaking phase) — on a `game_view` message, replaces the
    stored `GameView` snapshot; on `game_over`, sets a local flag `isGameOver()` reads
    and prints the message's `reason` (if any) to the console; on
    `opponent_disconnected`/`opponent_reconnected`, prints a console-only status line
    (no rendering work for either yet). Any other type, or any parse failure, is
    silently ignored (never throws out of a socket callback). All shared state is
    behind one `std::mutex`, since this callback runs on a different thread than
    `handlePixelClick`/`getGameView`/`isGameOver` (the render loop's thread) —
    same category of problem `CliShell`'s output mutex already solves, just for
    mutable game state instead of interleaved console output.

## Current state of integration

There is exactly one way to play now, and it requires two different logged-in users and
**four server-side processes** (**Phase 3**: `KungFuChessGameNode` is new; `KungFuChessServer`
is renamed `KungFuChessWsGateway`): start `KungFuChessApiGateway` (REST, port 9004),
`KungFuChessWsGateway` (WebSocket, port 9002), and `KungFuChessGameNode` (no client-facing
port, health check only on 9005) — all three pointed at the same user store and the same
Redis (`KUNGFUCHESS_REDIS_URL`, mandatory as of this phase — see the top-of-file Phase 3
review note and "Known gaps" below) — then run `KungFuChessClient` once per player (two separate
terminals/machines). Each client process: connects a `WebSocketClient`; runs `CliShell`
for `register`/`login`, which now call `ApiGatewayClient`'s REST endpoints (synchronous
HTTP, no WebSocket involved for `register` at all); on a successful REST `login`, sends
the returned token over the WebSocket as `LoginWithTokenRequest` and blocks on the
matching `login_result` (see `CliShell::run`); on a successful `login_result`, either
resumes an existing session (a reconnect — see below) or sends
`FindGameRequest` and blocks for `MatchFoundResult`/`NoMatchResult` (`waitForMatch` in
`client/src/main.cpp`, auto-retrying on a no-match after the user confirms); and once
matched, constructs `GameClient` (seeded with the match's initial `GameView`) and builds
`BoardCanvas`/`SpriteManager`/`AnimationFrame`/`Renderer`/`GameLoop` exactly as the old
`gui_main.cpp` did — all client-side phases still run in **one process, over one
`WebSocketClient` connection** (plus the one-off REST calls), which is what lets the
server treat "connection id" as "identity" for the whole session (see below). There is
no more separate GUI-only executable to run instead.

Real per-connection server push exists now: `server/src/network/WebSocketServer` (living in
the `KungFuChessWsGateway` process) tracks every open connection by id and exposes both
`broadcast(json)` (all connections) and `sendTo(connectionId, json)` (one). `GameSession`
(living in the separate `KungFuChessGameNode` process as of Phase 3) never calls
`sendTo` directly any more — its `SendToFn` closes over `GameNodePushPublisher::push`
instead, which publishes to Redis; the Gateway's `GameNodePushRouter` is what actually
calls `WebSocketServer::sendTo` on the other end. The *set* of pushes reaching exactly the
right connection(s) is unchanged from before this phase (`game_started`/`game_view`/
`game_over`, `match_found`/`no_match`/`opponent_disconnected`/`opponent_reconnected`) — only
the hop in the middle is new. Combined with the Game Node's own tick loop, a connected
client's board now updates continuously (an in-flight motion glides smoothly) whether or
not that client has sent a request recently — the old "only refreshes on your own move/jump"
limitation is gone.

**Auth gates everything.** `join_game`/`game_joined` no longer exist: a `GameSession` is
only ever created by `GameSessionManager::createSession`, only ever triggered by a
`Matchmaker` pairing (Phase 3: `gamenode/src/main.cpp`'s own tick loop calling it
directly, in the same process; Phase 4b: `gameallocator/src/main.cpp`'s tick loop calls
`GameAllocator::allocate`, which fires a fire-and-forget `GameNodeCreateSessionRequest`
to whichever shard it picked, and *that* shard's `GameNodeRequestRouter` is what
actually calls `createSession` now, in a different process from the one that decided
to) or, for a reconnect, from an existing session found by user id. A
`move`/`jump` request is resolved to a session via
`GameSessionManager::findSessionByConnection(connectionId)` — a connection with no
active session gets `{"error":"no_active_game"}`, and one that names a square holding
the *other* participant's piece (checked via the new `Controller::pieceColorAt`) gets
`{"error":"not_your_piece"}` before the engine is ever touched. See "Server /
persistence / networking layer" below for matchmaking pairing rules and the
disconnect/reconnect forfeit-and-resume design.

What's still missing/rough:
- **Sprite-level animation** still doesn't cover `Captured` (no on-disk sprite folder
  exists for it at all — see Gaps).
- No visual feedback for check/checkmate/stalemate (none of those are implemented at
  the rules level either — see Gaps).
- `wss://`/TLS still doesn't exist — plain `ws://` only.
- Matchmaking/session/reconnect state is entirely in-memory — a server restart drops
  every queued or in-progress game (scores persist in SQLite; games don't). No
  persistence-across-restart is planned yet.
- No spectators, no resign/draw offers, no rooms beyond 1v1 — all deliberately out of
  scope for now. (ELO-style rating itself is implemented — see
  `server/src/services/Rating/EloCalculator`/`RatingService` — this bullet is only about the
  still-missing social/matchmaking features.)

## Testing

doctest framework (single header, vendored as its own copy in each of
`server/tests/doctest.h`, `client/tests/doctest.h`, `protocol/tests/doctest.h` — kept
as separate copies rather than a shared location, since each is an independent build
target and this is a vendored single-header third-party file, not project code).

- `server/tests/` — `auth_service_test.cpp`, `sqlite_user_repository_test.cpp` (SQLite
  implementation, no sockets), `in_memory_user_repository_test.cpp` (same contract
  coverage against the in-memory implementation — see "Server / persistence /
  networking layer" below); `postgres_user_repository_test.cpp` (same contract again,
  against a real reachable Postgres — `KUNGFUCHESS_TEST_POSTGRES_URL`, defaulting to
  docker-compose's `postgres` service credentials; truncates `users` up front per
  `TEST_CASE` since there's no `:memory:`-style disposable database here, unlike SQLite's
  version; the whole file is a no-op unless `KUNGFUCHESS_HAS_POSTGRES` is defined);
  `game_session_test.cpp` (construction pushes to both
  participants by connection id, `not_your_piece`/`unknown_connection` rejection,
  per-tick pushes, disconnect suppresses sends to that slot only, reconnect resumes
  them, `tickAll`, `rebindConnection`) and `game_request_handler_test.cpp`
  (connection-scoped `move`/`jump` dispatch including `no_active_game`/`not_your_piece`
  errors) — both use a plain `InMemoryUserRepository` and a `LocalSessionIndexStore` now,
  since `GameSessionManager` requires an `IUserRepository&`/`ISessionIndexStore&`;
  `auth_request_handler_test.cpp` (`login_token` success/
  invalid-signature/expired/unknown-user against a real `TokenService` +
  `InMemoryUserRepository`, plus one test asserting `register`/`login` are no longer
  recognized message types at all, now that step B deleted those branches; **Phase 3**:
  its fixture now constructs a `LocalReconnectResolver` wrapping the same
  `GameSessionManager` it always built, passed to `AuthRequestHandler` in place of the
  raw `GameSessionManager&` — a construction-only change, no new assertions, proving the
  interface extraction is behavior-preserving);
  `matchmaker_test.cpp` (score-range pairing, earliest-queued-first ordering,
  `MAX_WAIT_MILLIS` timeout, idempotent `enqueue` — now against a `LocalMatchQueueStore`);
  `local_connection_store_test.cpp` (**Phase 1b** — `ConnectionRegistry` +
  `LocalConnectionStore`'s supersede-on-reauth/supersede-safe-disconnect semantics; the
  first dedicated test for this logic, which had none before); `redis_connection_store_test.cpp`/
  `redis_match_queue_store_test.cpp`/`redis_session_index_store_test.cpp` (**Phase 1b**
  — same contract coverage again, this time against a real reachable Redis,
  `KUNGFUCHESS_TEST_REDIS_URL` defaulting to docker-compose's `redis` service; each
  clears its own keys up front/after for isolation. Unlike the Postgres tests, these
  aren't behind a compile-time macro at all — hiredis/redis-plus-plus are vendored
  unconditionally, see "Build system" above — so they always compile and simply fail
  with a connection error if no Redis is reachable when the suite runs);
  `local_reconnect_resolver_test.cpp` (**new, Phase 3** — `LocalReconnectResolver`'s
  `checkAndRebind` against a real `GameSessionManager`: no-session/nullopt,
  active-session/rebind-and-resume-payload, and that only the checked user's slot is
  affected — pure in-process, no Redis);
  `game_node_push_router_test.cpp` (**new, Phase 3** — the actual Redis pub/sub round
  trip `GameNodePushRouter`/`GameNodePushPublisher` use in production: a published
  `"push"` reaches the router's default handler, `waitForReconnectCheckResult` both
  resolves once a reply arrives and times out cleanly if none ever does. Also against a
  real reachable Redis (same `KUNGFUCHESS_TEST_REDIS_URL`); note its file comment on why
  the router/Redis connection are built once as a process-lifetime `static` `Fixture`
  rather than per-`TEST_CASE` — destroying either while the router's background thread
  is still running is a real use-after-free, not a hypothetical one, see
  `GameNodePushRouter.h`); `common_tests/` (
  `EventBus`/`PieceStateToString`/`TimeProgress`/`TokenService` — the last one
  **Phase 2**: issue→verify round trip, expired/tampered/wrong-secret rejection); and
  `game_tests/` (mirrors
  `server/src/game/{model,rules,Engine,Controller,IO}`, same one-`TEST_CASE`-per-class/
  `SUBCASE`-per-scenario pattern, no mocking). All in one binary,
  `KungFuChessPersistence_tests` (name kept from before the split rather than renamed,
  since it now covers considerably more than persistence).
- `protocol/tests/` — round-trip `toJson()`→`fromJson()` coverage for every message
  struct, including `find_game`/`match_found`/`no_match`, `login_token`/`LoginResult`'s
  optional `token` field (**new, Phase 2**), and a full `GameViewMessage`
  round-trip through a real `BoardView`/`MotionView`/`JumpView`/`RestView` snapshot.
- `apigateway/tests/` (**new, Phase 2**) — `api_gateway_request_handler_test.cpp`:
  `ApiGatewayRequestHandler`'s `handleRegister`/`handleLogin` against a real
  `AuthService`/`InMemoryUserRepository`/`TokenService`, no real HTTP server or
  database — register success/duplicate, login issues a token that
  `TokenService::verify`s back to the same userId, wrong password issues no token,
  malformed JSON never throws.
- `client/tests/` (new) — `board_mapper_test.cpp` (updated for `BoardMapper`'s
  `PositionView` return type) and `game_client_test.cpp` (construction from a given
  initial `GameView`/default-state/no-crash-without-a-live-connection coverage only —
  `GameClient`'s actual click-to-select-then-move decision logic depends on real board
  content that only ever arrives from a live server's `GameViewMessage`, so that path,
  along with the full register/login/matchmake/color-assignment/disconnect/reconnect
  flow, is verified manually against a running server and two real client processes
  instead of faked in a unit test).

Notably **no tests for `client/src/ui/*`**, no tests for `CoordinateConverter`, and no
tests for the DTOs' `toJson()`/`fromJson()` round-trips *outside* of what
`protocol/tests` happens to exercise via `GameViewMessage` — worth adding direct
coverage for `BoardView`/`PieceView`/etc.'s own serialization independent of the
message wrapper.

## Server / persistence / networking layer (`server/`, `protocol/`, `client/`)

The full networked, auth-gated, matched design. `server/` hosts persistence,
matchmaking, the authoritative game engine, and the auth/matchmaking/game-command JSON
APIs; `protocol/` is the shared wire-format contract; `client/` is one executable that
runs all three phases (auth, matchmaking, game) over one connection.

### `protocol/` — shared JSON message contract

Header-only, depended on by `common/` (its `Message.h` wraps `common::GameView`),
`server/`, `client/`, and (**new, Phase 2**) `apigateway/`, so no one duplicates the
wire format — including the REST bodies below, which reuse these same structs rather
than inventing new ones.

- `MessageType.h` — the JSON envelope's `"type"` string constants: `register`,
  `login`, `register_result`, `login_result`, `error`, `move`, `jump`, `game_view`,
  `game_started`, `game_over`, `find_game`, `searching`, `match_found`,
  `no_match`, `opponent_disconnected`, `opponent_reconnected`, and (**new, Phase 2**)
  `login_token`. `join_game`/`game_joined`
  are gone — a session now only ever comes into being via a match (or a reconnect
  resume), both of which already hand the client a `match_found`-shaped payload
  carrying the initial `GameView`. `register`/`login` themselves are no longer sent
  over the WebSocket connection at all (`server/`'s `AuthRequestHandler` doesn't
  recognize them anymore) — they're kept only because `apigateway/`'s REST endpoints
  reuse `RegisterRequest`/`LoginRequest`/`RegisterResult`/`LoginResult` verbatim as
  their request/response *bodies*, where the `"type"` field is simply unused.
- `Message.h` — one struct per message, each with an in-class `toJson()`/`fromJson()`
  pair (built on `nlohmann::json`) — the only place that knows that message's field
  names. Auth structs: `RegisterRequest`/`LoginRequest`/`RegisterResult`/`ErrorResult`
  are unchanged; `LoginResult` gained one new optional field, **`token`** (**Phase 2**
  — empty unless the API Gateway's REST `/login` populated it; the WebSocket
  process's own `login_result` reply never sets it, since it only ever verifies a
  token, never issues one). New (**Phase 2**): `LoginWithTokenRequest{token}` — sent
  over the WebSocket connection once the client already holds a token from the REST
  `/login`, replacing the old WS-native password `LoginRequest` for that purpose.
  Game structs:
  `MoveRequest{fromRow, fromCol, toRow, toCol}`, `JumpRequest{row, col}`,
  `GameViewMessage{GameView view}` (wraps the DTO snapshot directly, calling its own
  `toJson()`/`fromJson()`), `GameStartedMessage`, `GameOverMessage{reason, winnerUserId}`
  (both fields optional/empty for an ordinary king-capture ending; populated for a
  disconnect-timeout forfeit — see `GameSession::forfeitTo`). Matchmaking structs (new):
  `FindGameRequest` (empty), `SearchingResult` (empty ack), `MatchFoundResult{sessionId,
  color, opponentUsername, GameView view}` (used both for a fresh match and for a
  reconnect resume), `NoMatchResult` (empty), `OpponentDisconnectedMessage`/
  `OpponentReconnectedMessage` (empty). `PieceColor` (used by `MatchFoundResult.color`)
  is serialized via the same `NLOHMANN_JSON_SERIALIZE_ENUM` registration
  `common/enums/EnumJson.h` already provides. `readType(json)` is unchanged.

### `server/` — persistence + matchmaking + game engine + WebSocket networking

- `server/src/persistence/*` — the repository-pattern layer. `UserRecord` (plain row
  DTO), `IUserRepository` (the interface every consumer depends on: `createUser`/
  `findByUsername`/`findById`/`setScore`), and `UserRepositoryExceptions.h`
  (`DuplicateUsernameException`, thrown by `createUser` on a duplicate username --
  backend-agnostic on purpose, so callers never need to catch a SQLite-specific type)
  live directly under `persistence/`, since every backend implements/throws against
  them. Each concrete backend gets its own subfolder — `Sqlite/`, `InMemory/`,
  `Postgres/` — and `Factory/` holds the one class that knows all three exist:
  `RepositoryFactory::createUserRepository(RepositoryBackend, sqliteDbPath,
  postgresConnectionString)` builds whichever backend is asked for.
  `IUserRepository` is deliberately ignorant of hashing: `createUser(username,
  passwordHash)` stores whatever credential string it's given (hashing is
  `AuthService`'s job, see below), and new users start at
  `RatingConfig::INITIAL_RATING` (an explicit insert value, not the schema's inert
  `DEFAULT 0`). `verifyPassword` is gone (moved to `AuthService`); `updateScore`
  (relative delta) is gone too, replaced by `setScore(userId, newScore)` (absolute --
  what an ELO update needs), called from `RatingService` (see below), which is in turn
  what every `GameSession`'s `GameOutcomeFn` closes over.
  - `Sqlite/SqliteDatabase` (renamed from the old flat-folder `Database`, unchanged in
    content) is no longer constructed anywhere outside `SqliteUserRepository.cpp` —
    `SqliteUserRepository` owns one internally (built from the `dbPath` string passed
    to its constructor) instead of taking one by reference, so it's the only class in
    the codebase that still includes `<SQLiteCpp/SQLiteCpp.h>` or knows a
    `SQLite::Statement` exists.
  - `InMemory/InMemoryUserRepository` — a `std::unordered_map<int, UserRecord>` + its
    own `std::mutex` (guards the map itself, since nothing else does for this
    implementation), no SQLite/file/network dependency at all. Exists both as a fast
    test double (see "Testing" above) and as concrete proof `IUserRepository` really
    decouples business logic from SQLite, not just in theory.
  - `Postgres/PostgresUserRepository` (Phase 1 of MIGRATION_PLAN.md) — implements the
    same contract against `PostgresDatabase` (owns a `pqxx::connection` + schema,
    mirroring `SqliteDatabase`'s role). Locks its own `std::mutex` around every method,
    unlike `SqliteUserRepository` — a single `pqxx::connection` isn't safe for
    concurrent use from multiple threads the way SQLiteCpp is. Both files' entire
    contents are guarded by `#ifdef KUNGFUCHESS_HAS_POSTGRES`, only ever defined when
    `server/CMakeLists.txt`'s `find_package(PostgreSQL QUIET)` actually finds libpq
    (always true in the Docker/Linux build; optional on a native Windows configure,
    see "Build system" below) — a build without it compiles both to empty translation
    units, and `RepositoryFactory::createUserRepository` throws instead of failing to
    compile if `RepositoryBackend::Postgres` is requested anyway.
  - `Factory/RepositoryFactory` is the single call site (`server/main.cpp`) that decides
    `RepositoryBackend::Sqlite` vs. `RepositoryBackend::InMemory` vs.
    `RepositoryBackend::Postgres` today — adding a fourth backend later means one new
    enum value plus one new `case` there, no changes to
    `AuthService`/`RatingService`/`GameSessionManager`, all of which only ever see
    `IUserRepository&`. `server/src/main.cpp` still passes `RepositoryBackend::Sqlite`
    unconditionally unless the `KUNGFUCHESS_POSTGRES_URL` environment variable is set, in
    which case it opts into `RepositoryBackend::Postgres` with that connection string —
    SQLite remains the actual default until a staging run proves Postgres out.
- `server/src/security/PasswordHasher` — **new**. The only class that knows a hashing
  algorithm exists: `hash(plaintext)`/`verify(plaintext, storedHash)`, wrapping a
  vendored bcrypt (`bcrypt_vendor` target in `server/CMakeLists.txt` -- fetched by
  commit via `FetchContent_Populate`, not `FetchContent_MakeAvailable`, since bcrypt's
  own `CMakeLists.txt` targets GCC/make with a GNU-assembler `.S` file and won't
  configure under the MSVC generator this project builds with; `x86.S` is excluded from
  `bcrypt_vendor`'s sources entirely -- `crypt_blowfish.c` only takes that asm path
  under `#ifdef __i386__`, a GCC-only macro MSVC never defines, so the portable C path
  always runs here regardless). Cost factor lives in
  `server/src/security/SecurityConfig.h` (`BCRYPT_COST_FACTOR`), not hardcoded inside
  `PasswordHasher.cpp`.
- `server/src/services/Auth/AuthService` — thin, networking-agnostic wrapper around
  `IUserRepository&` (never a concrete backend), its own mutex (multiple client
  connections can call in concurrently). Now also owns
  the one `PasswordHasher` used for both directions of credential handling:
  `registerUser` hashes before the repository ever sees the password; `login` verifies
  the stored hash against what the repository hands back. This is the seam where
  "hashing" and "persistence" are kept separate -- the repository never hashes or
  compares passwords itself. `registerUser` catches `DuplicateUsernameException` (not
  `SQLite::Exception` -- see `server/src/persistence/*` above) to detect a duplicate
  username, so this class has no SQLite-specific code or `#include` at all.
  **Phase 2**: this class's only caller is now `apigateway/`'s
  `ApiGatewayRequestHandler` -- `server/`'s own `AuthRequestHandler` was rewritten to
  never construct or call an `AuthService` at all (see below), which is why
  `KungFuChessAuth` (and, transitively, `KungFuChessSecurity`/`bcrypt_vendor`) is no
  longer linked into the `KungFuChessServer` executable (see "Build system" above).
- `common/Security/TokenService` (**new, Phase 2**) — issues/verifies a small
  homegrown signed token (deliberately not a real JWT): `issue(userId, nowMillis)`
  builds `base64url(json{"userId","exp"}) + "." + base64url(HMAC-SHA256(secret, payload))`;
  `verify(token, nowMillis)` recomputes the HMAC (constant-time compare) and checks
  `exp`, returning `{valid, userId, error}`. Built on `common/Security/Sha256.h`/`.cpp`,
  a small vendored (not FetchContent'd -- simple enough to just write out) HMAC-SHA256
  implementation. `issue()` runs only in `apigateway/`'s `ApiGatewayRequestHandler`
  (right after `AuthService::login` succeeds); `verify()` runs only in `server/`'s
  `AuthRequestHandler`. Both processes must be constructed with the same secret string
  (`KUNGFUCHESS_TOKEN_SECRET` env var, falling back to `common/Config/TokenConfig::
  DEV_INSECURE_DEFAULT_SECRET` if unset -- see "Known gaps" below). Callers pass in
  `wallClockMillis()` (`common/WallClock.h`), never `nowMillis()` (`common/
  MonotonicClock.h`) -- see that header's comment for why steady_clock doesn't work
  once issuer and verifier are different processes.
- `server/src/services/Connection/ConnectionRegistry` — mutex-guarded `connectionId ->
  {userId, username, score}` (plus the reverse `userId -> connectionId`), populated
  only by `AuthRequestHandler` on a successful login (`register` never authenticates
  a connection). This is the identity binding every later request on that connection is
  checked against — nothing ever trusts a client-claimed user id. `onDisconnected`
  clears both directions on a WebSocket close. **Phase 1b**: `AuthenticatedUser` moved
  to its own `AuthenticatedUser.h`; raw storage now lives behind `IConnectionStore&`
  (`set`/`erase`/`find`/`setUserConnection`/`eraseUserConnection`/`findConnectionForUser`
  — pure CRUD), injected at construction. `ConnectionRegistry` itself keeps its own
  mutex and the "a reconnect supersedes the old connection" read-then-write logic
  unchanged, just expressed in terms of the store's primitives instead of touching maps
  directly — that mutex is what keeps those multi-step sequences atomic, which remains
  sufficient as long as this stays one process (true cross-process atomicity, e.g. Lua
  scripts or `WATCH`/`MULTI`, is out of scope until a later phase actually splits into
  multiple processes sharing one Redis). `LocalConnectionStore` (today's actual default)
  ports the original two `unordered_map`s verbatim; `RedisConnectionStore` is the
  alternative (keys `conn:{connectionId}` -> JSON, `user_conn:{userId}` ->
  connectionId), both behind `sw::redis::Redis&`.
- `server/src/services/Matchmaking/Matchmaker` — a queue of `{connectionId, userId, username, score,
  enqueuedAtMs}` entries (the `Entry`/`Match` structs, hoisted to their own
  `MatchmakingTypes.h` in Phase 1b — `Matchmaker::Entry`/`Matchmaker::Match` still work
  everywhere via using-declarations). `tick(nowMs, onMatched, onTimedOut)` (called every
  server tick, see below) takes a snapshot of the queue and repeatedly pairs the
  earliest-queued entry with the closest-score entry still in it within
  `MatchmakingConfig::SCORE_RANGE` — `Match::first` is always the earlier-enqueued of the
  pair, which is exactly how "White = whoever entered first" is guaranteed with no extra
  bookkeeping — then reports anything that's waited past `MatchmakingConfig::
  MAX_WAIT_MILLIS` via `onTimedOut`, removing every matched/timed-out entry from the
  store afterward. No thread of its own; driven off the same tick cadence as
  `GameSessionManager::tickAll`. **Phase 1b**: queue storage moved behind
  `IMatchQueueStore&` (`add`/`remove`/`contains`/`all`) — the pairing/timeout algorithm
  itself is unchanged, now operating on the `all()` snapshot instead of a member
  `vector` directly, so `Matchmaker` no longer needs its own mutex at all (a concurrent
  `enqueue()` racing a `tick()`'s snapshot is a pre-existing-shape, benign race — that
  entry just waits for the next tick). `LocalMatchQueueStore` ports the original
  `vector<Entry>` + mutex verbatim (its own contains-then-push makes `add` idempotent);
  `RedisMatchQueueStore` is the alternative (one Redis Hash `matchqueue`, field =
  connectionId, value = JSON — `add` uses `HSETNX`, atomically idempotent for free).
- `server/src/game/` — the domain simulation (unchanged by this round of work — see
  Module map / Core domain model / Rules system / Real-time mechanics above), plus one
  small addition: `Controller::pieceColorAt(Position) -> PieceColor` (`None` if empty/
  out of bounds), added specifically so `GameSession` can check piece ownership before
  ever calling `move()`/`jump()`.
- `server/src/services/GameSession/GameSession` — owns one `EventBus` + one `GameEngine` (built via
  `GameFactory::createNewGame`) + one `Controller`, **plus** its two participants:
  `Player{userId, username, connectionId}` for `white`/`black` (`connectionId` mutable —
  replaced on reconnect), a per-color `xDisconnectedAtMs` (0 = connected), and a
  `finished` flag. Publishes `GameStartedEvent` itself right after construction (same
  contract the old `main.cpp` used to fulfill manually) and subscribes handlers for all
  four `Events.h` types, same as before, except every push now goes through
  `sendToParticipants(json)` — which calls the injected `SendToFn` for each of `white`/
  `black` **only if that slot isn't currently marked disconnected** — instead of a
  global `broadcast`. Key methods:
  - `requestMove(connectionId, from, to)` / `requestJump(connectionId, position)` —
    resolve the requester's color from `connectionId` (`unknown_connection` if it
    matches neither participant), check `controller.pieceColorAt(from-or-position)`
    against that color (`not_your_piece` if it doesn't match), and only then delegate to
    `controller.move`/`jump`. Returns a `CommandOutcome{accepted, reason}` instead of
    `void` — this is the actual "no one may move the other color" enforcement.
  - `tick(milliseconds)` — advances the engine as before, pushes a `GameViewMessage` to
    both connected participants, then checks (using `nowMillis()`, wall-clock, not the
    engine's own logical clock) whether a disconnected slot has exceeded
    `MatchmakingConfig::RECONNECT_GRACE_MILLIS`; if so, calls `forfeitTo(winner, loser)`.
  - `markDisconnected(connectionId)` / `markReconnected(userId, newConnectionId)` —
    flip a slot's disconnected timestamp and push `opponent_disconnected` (to whoever's
    left connected) / `opponent_reconnected` (to the *other* participant specifically,
    not back to the one who just reconnected).
  - `forfeitTo(winner, loser)` — sets `finished`, applies the ELO rating update via the
    injected `GameOutcomeFn(winnerUserId, loserUserId)`, pushes
    `GameOverMessage{"opponent_disconnected", winner.userId}`. The `GameOverEvent`
    subscriber (an ordinary king-capture win) now calls this exact same
    `GameOutcomeFn` too -- resolving winner/loser from the event's `loserColor` field --
    fixing a real gap where a normal win used to update no one's rating at all. Both
    endings therefore share one rating-update code path instead of each inventing its
    own.
  - `resumeInfoFor(userId) -> optional<ResumeInfo{color, opponentUsername}>` — lets
    `LocalReconnectResolver` (**Phase 3**: previously `AuthRequestHandler` directly)
    rebuild a `MatchFoundResult` for a reconnecting player without reaching into this
    class's internals.
  - `isFinished()` covers *both* endings (king capture and forfeit) — `GameSessionManager`
    uses it as the one signal for "safe to drop this session."
  - Every public method still locks its own `std::mutex`, for the same reason as before
    (tick-loop thread + request-handling thread both touch the same `GameEngine`).
- `server/src/services/GameSession/GameSessionManager` — owns every active `GameSession` keyed by
  id (this map itself stays in-process only, unaffected by Phase 1b — see below), plus
  two reverse indexes (`connectionId -> sessionId`, `userId -> sessionId`) for O(1)
  lookup, all behind its own mutex. `getOrCreateDefaultSession` is gone —
  `createSession(whitePlayer, blackPlayer)` is the only way a session comes into being,
  called only from a `Matchmaker` match. `findSessionByConnection`/`findSessionByUserId`
  back `GameRequestHandler`'s dispatch and `LocalReconnectResolver`'s reconnect check
  (**Phase 3**: previously `AuthRequestHandler`'s directly).
  `rebindConnection(userId, newConnectionId)` and `onConnectionClosed(connectionId)`
  (which also drops that connection's index entry, so a stale route can never linger)
  wire reconnect/disconnect through to the right `GameSession`. `tickAll` now also
  removes any session `isFinished()` reports, along with its index entries. Constructed
  with a `SendToFn` (shared by every session it creates), an `IUserRepository&`, from
  which it builds one `RatingService` member (see below) that every session it creates
  gets a `GameOutcomeFn` closing over, and (**Phase 1b**) an `ISessionIndexStore&`
  covering just the two lookup indexes above (`bindSession`/`bindConnection`/
  `unbindConnection`/`unbindSession`/`findSessionIdByConnection`/`findSessionIdByUserId`)
  — deliberately *not* the `sessions` map itself, since a live `GameEngine` can't be
  externalized without a much bigger redesign reserved for a later phase.
  `LocalSessionIndexStore` (today's actual default) ports the original two
  `unordered_map`s verbatim, including `removeFinishedSessions`' linear "erase every
  entry whose value equals this session" scan (now `unbindSession`). `RedisSessionIndexStore`
  is the alternative (keys `session_conn:{connectionId}`/`session_user:{userId}` ->
  sessionId) — it also maintains a private auxiliary Redis Set `session_keys:{sessionId}`
  (the exact keys created for that session) purely so `unbindSession` can clean up via
  `SMEMBERS`+`DEL` instead of a `SCAN`/`KEYS` over the whole keyspace; `LocalSessionIndexStore`
  doesn't need this since an in-memory linear scan is already cheap.
- `server/src/services/Rating/EloCalculator` — **new**. Pure, stateless ELO math (no DB, no
  networking, no locking): `applyResult(ratingWinner, ratingLoser) -> Outcome{
  newWinnerRating, newLoserRating}`, using the standard logistic expected-score curve
  and `common/Config/RatingConfig::K_FACTOR`. Deliberately separate from
  `RatingService` so the arithmetic itself stays trivially unit-testable without an
  `IUserRepository` in the loop.
- `server/src/services/Rating/RatingService` — **new**. The single place that turns "who won"
  into "what changed in the DB": `applyGameResult(winnerUserId, loserUserId)` reads both
  users' current ratings via `IUserRepository::findById`, computes new ones via
  `EloCalculator`, and persists both via `IUserRepository::setScore`. This is what
  replaced the old flat `MatchmakingConfig::SCORE_DELTA_WIN`/`_LOSS`, and — since both
  `GameSession::forfeitTo` and its `GameOverEvent` subscriber call the same
  `GameOutcomeFn` (see above) — the only rating-update code path in the whole system,
  covering both game-ending shapes.
- `server/src/handlers/AuthRequestHandler` — **rewritten in Phase 2, adjusted in
  Phase 3**. Only recognizes `login_token` now (`register`/password-based `login` are
  gone entirely -- they threw `unknown_type` the moment step B landed, and
  `server/tests/auth_request_handler_test.cpp` asserts exactly that): verifies the token
  via `TokenService::verify` (constructor dependency, not `AuthService` -- this class no
  longer depends on `AuthService` at all), then reads the user's username/score via
  `IUserRepository::findById(verified.userId)` purely for display/`ConnectionRegistry`
  purposes, never a password field. On success, the shared `completeLogin(connectionId,
  userId, username, score)` tail (1) records `connectionId -> user` in
  `ConnectionRegistry`, and (2) calls `IReconnectResolver::checkAndRebind(userId,
  connectionId)` (**Phase 3**: this used to be a direct `GameSessionManager&` call --
  see "Server-owned tick loop moved to gamenode/" below for why it's an interface now)
  — if this returns a `protocol::MatchFoundResult` (a reconnect), pushes it via an
  injected `SendFn` *before* returning the `login_result` reply (so it's guaranteed to
  arrive first on the same connection). This is what makes it safe for the client to
  always auto-`find_game` right after a successful login — a returning player is
  already back in their game by the time that request would arrive. See the
  top-of-file Phase 2 review note for the full auth-split before/after, and the
  Phase 3 review note for the `IReconnectResolver` extraction.
- `server/src/services/GameNodeBridge/` (**new, Phase 3**) — the Redis pub/sub seam
  between the `KungFuChessWsGateway` and `KungFuChessGameNode` processes, all built on
  the already-vendored redis-plus-plus (`sw::redis::Redis::publish`/`subscriber()`), and
  all speaking the two internal-only envelope structs in `GameNodeMessages.h`
  (`GameNodeRequest`/`GameNodePush` -- deliberately not `protocol/`, since no client ever
  sees these):
  - `GameNodeRequestPublisher` (Gateway side) — fire-and-forget `PUBLISH` to
    `GameNodeConfig::REQUESTS_CHANNEL` for `forward` (a raw client request),
    `notifyConnectionClosed`, and `requestReconnectCheck`.
  - `GameNodeRequestRouter` (Game Node side) — the other end: a background thread
    consuming `REQUESTS_CHANNEL`, running the same matchmaking-then-game dispatch
    `server/main.cpp`'s `dispatch()` used to run for a `"client_message"`, calling
    `matchmaker.removeByConnection`/`sessionManager.onConnectionClosed` for a
    `"connection_closed"`, and answering a `"reconnect_check"` via the
    `IReconnectResolver` it was constructed with.
  - `GameNodePushPublisher` (Game Node side) — the reverse direction's publisher:
    `push` (a raw protocol/ message, `PUBLISH`ed to `GameNodeConfig::PUSHES_CHANNEL`)
    and `pushReconnectCheckResult`. This is what every `GameSession`'s `SendToFn` and
    the tick loop's `onMatched`/`onTimedOut` lambdas call now, instead of
    `WebSocketServer::sendTo` directly (see below).
  - `GameNodePushRouter` (Gateway side) — a background thread consuming
    `PUSHES_CHANNEL`; a `"push"` goes to its default handler (`WebSocketServer::sendTo`
    in `server/main.cpp`); a `"reconnect_check_result"` instead fulfills whichever
    `waitForReconnectCheckResult()` call is currently waiting on that connectionId
    (a `condition_variable`-based waiter keyed by connectionId, registered *before* the
    matching request is published via an `afterRegistered` callback, so a
    pathologically fast reply can never race ahead of the wait being ready for it).
    **Lifetime note** (see the class's own header comment and
    `server/tests/game_node_push_router_test.cpp`'s file comment): this class and the
    `Redis&` it's given are meant to live for the whole process, exactly like the tick
    thread below — destroying either while its background thread might still be running
    is a real use-after-free, not just bad practice.
  - `IReconnectResolver`/`LocalReconnectResolver`/`RemoteReconnectResolver` — see the
    top-of-file Phase 3/4b review notes; `LocalReconnectResolver` wraps a real
    `GameSessionManager&` (used inside `gamenode/` itself, and by
    `auth_request_handler_test.cpp`), `RemoteReconnectResolver` (used by the real
    `KungFuChessWsGateway`) first asks `IGameShardRoutingStore::findShardForUser` which
    shard (if any) hosts userId's session -- no entry means "no active session," no
    Redis round trip at all -- and only then does the blocking
    `GameNodePushRouter`/`GameNodeRequestPublisher` round trip described above, against
    that specific shard's channel.
  - `GatewayGameRouter` (Gateway side, **Phase 4b**, replacing Phase 3's
    `RemoteGameNodeHandler`) — same `handle(connectionId, rawJson) -> std::string` shape
    `MatchmakingRequestHandler`/`GameRequestHandler` already have, so `server/main.cpp`'s
    `dispatch()` barely changed again, but this class does real work Phase 3's version
    didn't: it parses just the request's `"type"` and sends `find_game` to
    `GameNodeConfig::MATCHMAKING_REQUESTS_CHANNEL` (the one Allocator) while resolving
    `move`/`jump` to a specific shard's channel via `ISessionIndexStore::
    findSessionIdByConnection` + `IGameShardRoutingStore::findShardForSession` (both
    read-only lookups against Redis-backed stores this class never writes to). Always
    returns `"{}"` synchronously either way (an inert, typeless JSON object --
    `WebSocketServer::onMessage` always sends the handler's return value back over the
    socket, so this can't return an empty string without sending an empty frame; no
    client-side code parses or reacts to it). The *real* reply (whatever
    `MatchmakingRequestHandler`/`GameRequestHandler` actually produce once the right
    process runs the request) arrives later over `PUSHES_CHANNEL`, same as every other
    async push always has -- verified by reading `GameClient.cpp`/`client/src/main.cpp`
    that neither the move/jump path nor `waitForMatch` ever depended on move/jump/
    find_game's old synchronous reply. `notifyConnectionClosed` fans out to *both* the
    Allocator's channel (in case the connection was only ever queued, never matched --
    `Matchmaker::removeByConnection` is a no-op otherwise) and the resolved shard's
    channel if any (`GameSessionManager::onConnectionClosed`).
  - `GameAllocatorRequestRouter` (Game Allocator side, **new, Phase 4b**) — the
    find_game-handling half of what Phase 3's `GameNodeRequestRouter` used to be:
    `client_message` -> `MatchmakingRequestHandler::handle` (find_game only ever arrives
    here), `connection_closed` -> `Matchmaker::removeByConnection`.
- `server/src/handlers/MatchmakingRequestHandler` — parallel to
  `AuthRequestHandler`/`GameRequestHandler`: `find_game` requires an authenticated
  connection (`ConnectionRegistry`) with no existing session (`already_in_game`
  otherwise, **Phase 4b**: checked via `ISessionIndexStore::findSessionIdByConnection`
  instead of a direct `GameSessionManager::findSessionByConnection` call, since this
  class no longer shares a process with any `GameSessionManager`) and enqueues it into
  `Matchmaker`, replying with `SearchingResult` — the actual `match_found`/`no_match`
  outcome is always a later, unsolicited push from the tick loop (see below), never this
  handler's synchronous reply. **Phase 4b**: now constructed inside
  `gameallocator/src/main.cpp` (moved from `gamenode/`), invoked from
  `GameAllocatorRequestRouter`.
- `server/src/handlers/GameRequestHandler` — dispatches `move`/`jump` by resolving
  `GameSessionManager::findSessionByConnection(connectionId)` first (`no_active_game` if
  none), then calling the connection-aware `requestMove`/`requestJump` and translating a
  rejected `CommandOutcome` into `ErrorResult{reason}`. Never throws. Still constructed
  only inside `gamenode/src/main.cpp` (unaffected by the Phase 4b split, since this
  class always needed a real `GameSessionManager&`, which stays shard-side).
- **Matchmaking tick loop, in `gameallocator/`** (**Phase 4b**, was `gamenode/`'s own
  tick loop before this phase, and `server/src/main.cpp`'s before Phase 3) — a detached
  `std::thread` sleeping `TimingConfig::SERVER_TICK_INTERVAL_MILLIS` then calling
  `matchmaker.tick(nowMillis(), onMatched, onTimedOut)`, forever. `onMatched` now calls
  `GameAllocator::allocate` (fire-and-forget session creation on whichever shard gets
  picked, plus a locally-computed deterministic initial `GameView` -- see the top-of-file
  Phase 4b review note) and `pushPublisher.push`es each side a `MatchFoundResult` using
  the returned `Allocation`; `onTimedOut` pushes `NoMatchResult` the same way. Session
  *ticking* (`sessionManager.tickAll(...)`) is a **separate** tick loop now, one per Game
  Server Shard (`gamenode/src/main.cpp`) -- Phase 4b split what used to be one combined
  loop into two, since `Matchmaker` and `GameSessionManager` no longer live in the same
  process.
- `server/src/main.cpp` (the `KungFuChessWsGateway` executable, **renamed from
  KungFuChessServer in Phase 3**) composes `AuthRequestHandler` and `GatewayGameRouter`
  behind one `dispatch()` function: tries auth, and anything it doesn't recognize is
  routed by `GatewayGameRouter` — `MatchmakingRequestHandler`/`GameRequestHandler` are
  never constructed here at all (see `gameallocator/src/main.cpp`/`gamenode/src/main.cpp`
  below). `server.setCloseHandler` notifies `ConnectionRegistry` locally and calls
  `GatewayGameRouter::notifyConnectionClosed`, which fans the notification out to both
  the Allocator and (if routed) a specific shard, so `Matchmaker::removeByConnection`/
  `GameSessionManager::onConnectionClosed` still fire for every dropped connection, same
  as before this phase just across two possible process boundaries now instead of one.
- `gameallocator/src/main.cpp` (**new, Phase 4b**, the `KungFuChessGameAllocator`
  executable) — constructs `Matchmaker`/`MatchmakingRequestHandler`/`GameAllocator`
  exactly as `gamenode/src/main.cpp` used to before this phase, plus
  `GameAllocatorRequestRouter`/`GameNodeRequestPublisher`/`GameNodePushPublisher` in
  place of the shard-facing pieces it no longer needs, plus its own `HealthCheckServer`
  on `NetworkConfig::GAME_ALLOCATOR_HEALTH_CHECK_PORT` (9006). `KUNGFUCHESS_REDIS_URL` is
  mandatory (fails fast with a clear log line if unset), same reasoning as `gamenode/`
  below. Exactly one replica of this process ever runs.
- `gamenode/src/main.cpp` (Phase 3, narrowed by **Phase 4b**, the `KungFuChessGameNode`
  executable) — constructs `GameSessionManager`/`GameRequestHandler`/
  `LocalReconnectResolver` (no longer `Matchmaker`/`MatchmakingRequestHandler`/
  `GameAllocator`/`ConnectionRegistry` -- moved to `gameallocator/` above), plus
  `GameNodeRequestRouter`/`GameNodePushPublisher` in place of a `WebSocketServer` it
  doesn't hold, plus its own `HealthCheckServer` on `NetworkConfig::
  GAME_NODE_HEALTH_CHECK_PORT` (9005). Reads `KUNGFUCHESS_SHARD_ID` (default `"shard-1"`)
  and registers it with a `RedisShardLoadStore` at startup; subscribes to
  `GameNodeConfig::shardRequestsChannel(shardId)`, not a single shared channel like Phase
  3's version did. Unlike `server/src/main.cpp`'s still-opt-in `KUNGFUCHESS_REDIS_URL`,
  this process treats it as mandatory (fails fast with a clear log line if unset) -- its
  `GameSessionManager`'s session-index store must be the exact same Redis-backed state
  every other process's is, or `find_game`/reconnect break outright (see the top-of-file
  Phase 3/4b review notes).
- `server/CMakeLists.txt` — see "Build system" above for the full target graph.
  `KungFuChessGameSession` is deliberately its own target, not folded into
  `KungFuChessAuth`, even though both live under `src/services/` — a library named
  "Auth" building `GameSession` would be a misleading name for what it contains.

### `client/` — one executable, three phases over one connection (plus one REST hop)

- `client/src/network/WebSocketClient` — unchanged; `setOnMessage(MessageHandler)`
  lets the message handler be replaced after construction, which is exactly how control
  hands off between phases: `CliShell` installs itself first (auth), `main.cpp`'s
  `waitForMatch` installs itself next (matchmaking), and `GameClient` installs itself
  last (the game) — each one fully replacing the last, never composing handlers.
- `client/src/network/ApiGatewayClient` (**new, Phase 2**) — wraps `ix::HttpClient`
  (the one file that does), mirroring `WebSocketClient`'s framing. `registerUser(username,
  password)`/`login(username, password)` are synchronous REST calls against
  `apigateway/`'s `/register`/`/login` — reusing `protocol::RegisterRequest`/
  `RegisterResult`/`LoginRequest`/`LoginResult` for the bodies, same as the gateway
  itself. Makes no WebSocket calls at all; `login`'s returned `LoginResult.token` is
  what the caller sends over `WebSocketClient` next.
- `client/src/cli/CliShell` — the auth phase. **Phase 2**: `register`/`login` commands
  now call `ApiGatewayClient` (synchronous HTTP, no WebSocket round-trip needed for
  `register` at all anymore). On a successful `login`, sends
  `LoginWithTokenRequest{token}` over the `WebSocketClient` connection and blocks (via
  a `std::condition_variable`, with a timeout) on the `login_result` that follows —
  that part still genuinely belongs on the socket, since it's what binds this
  connection's identity — to return a `LoginOutcome{loggedIn, userId, username, score,
  resumedMatch}` to the caller instead of firing the request and leaving it to guess.
  `resumedMatch` is populated if a `match_found` push (a reconnect resume — always
  delivered before `login_result` on the same connection, see `AuthRequestHandler`
  above) arrived during the same login. `CliShell`'s constructor now takes an
  `ApiGatewayClient&` alongside `WebSocketClient&`.
- `client/src/main.cpp` — the whole client entry point now (`gui_main.cpp` is gone).
  Constructs `ApiGatewayClient` (pointed at `NetworkConfig::API_GATEWAY_PORT`,
  **Phase 2**) alongside `WebSocketClient`, passes both into `CliShell`. Connects, runs
  `CliShell::run()`; if not logged in, exits. If `resumedMatch` was
  already populated, skips matchmaking entirely and goes straight to the game. Otherwise
  calls `waitForMatch` (installs its own handler, sends `FindGameRequest`, blocks on a
  condition variable for `match_found`/`no_match`, auto-retrying — after the user
  presses Enter — on a `no_match`). Once matched, `runGame` constructs `GameClient` (with
  the match's initial `GameView`) and the same `BoardCanvas`/`SpriteManager`/
  `AnimationFrame`/`Renderer`/`GameLoop` stack `gui_main.cpp` used to build, then runs
  `GameLoop`.
- `client/src/ui/*` + `client/src/game/{BoardMapper,GameClient}` — unchanged in their
  own right; see "UI / rendering layer" and "`client/src/game/`" above.

Not yet done: `wss://`/TLS; request/response correlation IDs (fine today since each
client only ever has one request in flight at a time); session persistence across a
server restart; spectators; resign/draw. (Password hashing and ELO-style rating are
both implemented now — see "Server / persistence / networking layer" above.)

## Known gaps / things to be careful about when editing

1. ~~Logic layer isn't wired into the graphical executable~~ — long since fixed, and
   then superseded entirely: the graphical executable itself (the old local, no-network
   `KungFuChess`) is now retired, and so is the later GUI-only `KungFuChessGuiClient` —
   the one client executable, `KungFuChessClient`, runs auth then matchmaking then the
   game pane, all over the network, in one process.
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
   `BoardCanvas`/`SpriteManager` construction calls in `client/src/main.cpp` (moved here
   from the now-deleted `gui_main.cpp`).
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
11. ~~No real server→client push transport~~ — fixed: `WebSocketServer::sendTo`/
    `broadcast` plus per-`GameSession` participant connection ids give real, targeted,
    continuous push (see "Server / persistence / networking layer" above).
12. `common/DTO`'s converting-from-domain constructors must stay in their own
    separate `*FromDomain.cpp` files, not be merged back into each DTO's main `.cpp` —
    see "DTO layer" above for exactly why, and what silently breaks if this is ignored.
13. **Updated, from Phase 1b**: the seam described here now exists —
    `ConnectionRegistry`/`Matchmaker`/`GameSessionManager`'s lookup indexes are each
    swappable (`IConnectionStore`/`IMatchQueueStore`/`ISessionIndexStore`, Local by
    default, Redis as the alternative — see "Server / persistence / networking layer"
    above) — but **local/in-memory storage remains the actual default** everywhere
    unless `KUNGFUCHESS_REDIS_URL` is set, so a server restart still silently drops
    every queued player and every in-progress game by default (only SQLite/Postgres-backed
    scores survive either way). `GameSessionManager`'s `sessions` map of live
    `GameSession` objects was deliberately left out of this seam entirely — a
    `GameEngine` can't be made Redis-backed without a much bigger redesign (serializing/
    resuming live real-time engine state across a process), reserved for whichever later
    phase actually needs it.
14. **New**: a reconnect (`AuthRequestHandler`'s `login`-time check) racing the same
    session's forfeit-by-timeout (`GameSession::tick`, on the server's tick thread) is
    last-writer-wins, not arbitrated — an accepted edge case at this scale, flagged here
    so it isn't mistaken for an oversight later.
15. **New**: `GameSession::tick`'s disconnect-grace check uses `nowMillis()`
    (`common/MonotonicClock.h`, wall-clock `steady_clock`), completely independent of
    the `milliseconds` argument that advances the game engine's own logical clock —
    easy to conflate when reading `tick()`, since both fire from the same call.
16. **Updated, from Phase 2**: `docker-compose.yml`'s `redis` service is still a
    placeholder — nothing talks to it yet (reserved for Phase 1b's Redis-backed
    `ConnectionRegistry`/`Matchmaker`/`GameSessionManager` index implementations, not yet
    done; still a single Game Node either way, Phase 3+ territory). `postgres` is
    **no longer optional for the compose stack**: both `server` and the new
    `apigateway` service now have `KUNGFUCHESS_POSTGRES_URL` set (pointing at the same
    `postgres` service) in `docker-compose.yml` — this is the point Phase 2 flips
    Postgres from "reachable but SQLite is still the real default" to "actually
    required for the two containers to agree on who's registered." A **native, non-Docker**
    two-process dev run (`KungFuChessServer`/`KungFuChessApiGateway` run directly, no
    compose) does *not* get this for free — both default independently to their own
    local `kungfuchess.db` SQLite file unless you export `KUNGFUCHESS_POSTGRES_URL`
    yourself on both processes (see item 19 below).
17. **New, from the bcrypt/ELO change**: `GameEngine::executeMove`'s king-capture branch
    reads `destinationPiece`'s color for `GameOverEvent` -- `destinationPiece` is a raw
    pointer into `Board`'s piece vector, and `getBoard().removePiece(motion.getTo())`
    (called earlier in the same function, for the ordinary-capture case) invalidates
    it. The color is captured into a local (`capturedKingColor`) *before* that removal,
    specifically so `GameOverEvent{capturedKingColor}` never dereferences a dangling
    pointer -- if you touch this function again, keep reading anything off
    `destinationPiece` before the `removePiece` call, not after.
18. **New, from Phase 2**: `KUNGFUCHESS_TOKEN_SECRET` falls back to
    `common/Config/TokenConfig::DEV_INSECURE_DEFAULT_SECRET` (a fixed, checked-in
    string) when unset on either `server/` or `apigateway/` — fine for a zero-config
    native/local run (both processes fall back to the exact same constant), but a
    real, security-sensitive deployment must set this env var explicitly to a real
    secret on *both* processes, since anyone who knows the dev default could forge a
    valid `login_token` for any userId. `docker-compose.yml` currently sets it to a
    placeholder string (`change-me-to-a-real-secret-before-any-real-deployment`) as a
    reminder, not a real secret.
19. **From Phase 2**: `KungFuChessApiGateway` (writes users via `AuthService`)
    and `KungFuChessWsGateway` (reads them via `IUserRepository::findById` for the
    `login_token` path) are two independent processes that must be configured to
    point at the *same* user store, or they silently diverge -- e.g. a user
    registered against one process's local SQLite file is invisible to the other's.
    `docker-compose.yml` handles this by setting the same `KUNGFUCHESS_POSTGRES_URL`
    on all three server-side services (see item 16 above, and item 21 below for
    `KungFuChessGameNode` specifically); a native, non-Docker run must do the same by
    hand (export the same `KUNGFUCHESS_POSTGRES_URL` before starting all of them), or
    accept that registering against one and logging in against another (a
    `login_token` for a userId that repository has never heard of) fails with
    `"unknown_user"`.
20. **New, from Phase 3**: `AuthRequestHandler::completeLogin`'s reconnect check
    (`IReconnectResolver::checkAndRebind`) is the one place this phase keeps a
    synchronous-feeling cross-process call -- `RemoteReconnectResolver` blocks the
    calling thread (one of `WebSocketServer`'s own message-handling threads) for up to
    `GameNodeConfig::RECONNECT_CHECK_TIMEOUT_MILLIS` (3 seconds) waiting for
    `gamenode/`'s answer over Redis. A timeout is treated identically to "no active
    session" -- the login still succeeds, the player just doesn't get resumed into
    their old game and falls through to ordinary matchmaking instead -- so a slow or
    momentarily-unreachable Game Node degrades gracefully rather than hanging a login
    outright, but it does mean a *real* reconnect can be silently missed if the Game
    Node is unresponsive for the whole timeout window. Separately, and unchanged from
    before this phase (previously about a same-process race, now a genuine
    cross-process one): a reconnect racing that same session's forfeit-by-timeout
    (`GameSession::tick`, running on `gamenode/`'s own tick thread) is last-writer-wins,
    not arbitrated -- still an accepted edge case at this scale.
21. **New, from Phase 3**: `KUNGFUCHESS_REDIS_URL` flips from an explicit opt-in
    (Phase 1b's framing: "local/in-memory storage remains the actual default") to
    **mandatory** for both `KungFuChessWsGateway` and `KungFuChessGameNode` -- both
    processes fail fast with a clear log line and a non-zero exit if it's unset, rather
    than each silently falling back to its own local `ConnectionRegistry`/
    `IMatchQueueStore`/`ISessionIndexStore`, which would make the two processes
    invisible to each other (`find_game` would never work: `gamenode/`'s
    `MatchmakingRequestHandler` calls `connectionRegistry.find(connectionId)` against
    its *own* store, which would never see what the Gateway's `AuthRequestHandler`
    wrote to its own separate one). `docker-compose.yml` sets the same
    `KUNGFUCHESS_REDIS_URL` (`tcp://redis:6379`) on both services; a native,
    non-Docker run must do the same by hand, exactly like item 19 above for Postgres.
    `GameSessionManager`'s own `sessions` map of live `GameSession` objects is still
    the one piece of state that stays in-process only (now `gamenode/`'s process, not
    shared via Redis at all) -- unaffected by this, and still the reason a Game Node
    crash loses every session it was hosting outright (see SERVER_DESIGN.md's "Known
    Gaps" -- still true, still unaddressed until whichever later phase adds session
    snapshotting).
22. ~~`IShardLoadStore::decrementLoad`/`IGameShardRoutingStore::unbindSession` are never
    called anywhere~~ -- **fixed in Phase 4c**: `GameSessionManager` gained a defaulted
    `SessionFinishedFn` constructor parameter (no-op by default, so no existing
    caller/test needed updating), invoked from `removeFinishedSessions` with the
    finishing session's id and both participants' userIds
    (`GameSession::userIds()`, new); `gamenode/src/main.cpp` wires it to
    `shardRoutingStore.unbindSession`/`shardLoadStore.decrementLoad`. This was
    originally flagged in Phase 4a as harmless-with-one-shard bookkeeping debt, and
    became worth fixing in Phase 4c specifically because `ShardHealthMonitor` would
    otherwise misforfeit a long-finished game's stale routing entry the next time it
    scanned a shard that later actually crashed. `KUNGFUCHESS_SHARD_ID` is read but not
    enforced unique -- nothing stops two Game
    Node processes from both defaulting to `"shard-1"` and silently sharing one
    load-store entry/one routing identity; `docker-compose.yml`'s `gamenode-1` service
    sets it explicitly, but a native, non-Docker run of two shards side by side must set
    distinct values by hand, or `GameAllocator`'s "pick the least-loaded shard" logic
    degrades into a coin-flip between indistinguishable identities.
23. **New, from Phase 4b**: the Game Allocator is a genuine single point of failure now
    -- unlike a Game Server Shard (where losing one only drops the games *it* was
    hosting, see item 21 above), losing the one Allocator process stops *all* new
    matches from forming cluster-wide (existing in-flight games on already-allocated
    shards are unaffected, since a shard never talks to the Allocator once a session
    exists). This is an accepted, temporary shape for this sub-phase -- MIGRATION_PLAN.md
    Phase 4's own framing calls this "the one true singleton in the topology" -- not an
    oversight; a highly-available Allocator (active/passive failover, or making it
    stateless enough to run more than one safely) is out of scope until whichever later
    phase actually needs that resilience.
24. **New, from Phase 4b**: `GatewayGameRouter::handle` drops a `move`/`jump` request
    silently (just logs a warning) if `IGameShardRoutingStore` has no entry for the
    requesting connection's session -- this can legitimately happen if a shard crashed
    between the session being created and this lookup (see item 23's sibling concern for
    shards), or if a client sends a move for a session it was never actually part of. The
    client isn't told either way (same "never blocks on a synchronous move/jump reply"
    contract every prior phase already established) -- it'll notice via the *absence* of
    a following `game_view` push, not an explicit error. Fine at this scale; a real
    deployment would want this counted/alerted on (Phase 5 observability territory).
25. **New, from Phase 4c**: a shard is presumed dead purely from a lapsed heartbeat --
    there's no distinction between "actually crashed" and "briefly unable to reach Redis
    while still running fine otherwise" (a network partition between the shard and
    Redis, not a process crash). `SHARD_HEARTBEAT_TTL_MILLIS` being several multiples of
    `SHARD_HEARTBEAT_INTERVAL_MILLIS` (6s vs. 2s) narrows this window but doesn't close
    it; a shard that reconnects to Redis after being presumed dead has already had its
    sessions forfeited and itself `forget()`-ten, and will silently keep ticking games
    that the rest of the system now considers gone (its own `GameSessionManager` was
    never told anything happened) until a client's own dropped connection eventually
    surfaces the problem some other way. A real deployment would want this shard to
    also notice "I've been forgotten" (e.g. by checking `IShardLoadStore::knownShards()`
    still contains its own id) and exit/re-register rather than keep running as an
    orphan -- not done here, since it's a fundamentally different failure mode
    (false-positive death, not a real crash) than this sub-phase's own scope.
26. **New, from Phase 4c**: a shard-crash forfeit applies no ELO rating change at all
    (see `ShardHealthMonitor`'s class comment for why this is deliberate, not an
    oversight) -- both players just see the game end with `reason:
    "shard_unavailable"` and no winner. This is a real, if narrow, difference from
    every other game-ending path in this codebase (an ordinary king-capture win and a
    disconnect-timeout forfeit both update rating); flagged here so a future change
    that wants crash-forfeits to count competitively knows exactly where that decision
    currently lives.
27. **New, from Phase 4c**: `ShardHealthMonitor::checkAndForfeitDeadShards` scans every
    known shard on every call (`IShardLoadStore::knownShards()`), each requiring at
    least one Redis round trip (`isAlive`) plus, for a dead one, a `sessionsForShard`/
    per-session `usersForSession` call -- fine at today's scale (a handful of shards),
    but a real fleet-sized deployment would want this to shard its own scanning work
    or move to Redis keyspace notifications instead of polling, a Phase 5
    observability/scale concern, not a correctness one.

## Where to look for X

| Task | Start here |
|---|---|
| Change/add a piece's movement rule | `server/src/game/rules/<Piece>Rule.cpp`, registered in `RuleEngine`'s constructor |
| Change move/jump timing (cooldowns, speed) | `common/Config/TimingConfig.h`, `server/src/game/Engine/GameEngine.cpp`, `server/src/game/Controller/RealTimeArbiter.cpp` |
| Change the post-move (`RestKind::Long`) or post-jump (`RestKind::Short`) rest/cooldown duration or its guard | `common/Config/TimingConfig.h`, `server/src/game/Engine/GameEngine.cpp` (`settleCompletedMotions`/`settleCompletedJumps`/the ambush branch of `executeMove`), `server/src/game/Controller/RealTimeArbiter.cpp` |
| Change what happens when a move completes (capture, promotion, game-over) | `server/src/game/Engine/GameEngine.cpp` (`executeMove`) |
| Change click/selection UX (server-side REPL) | `server/src/game/Controller/Controller.cpp` (`click`) |
| Change the direct from/to move command the network uses | `server/src/game/Controller/Controller.cpp` (`move`), `server/src/handlers/GameRequestHandler.cpp` |
| Change who's allowed to move/jump which piece | `server/src/game/Controller/Controller.cpp` (`pieceColorAt`), `server/src/services/GameSession/GameSession.cpp` (`requestMove`/`requestJump`); tests in `server/tests/game_session_test.cpp`, `server/tests/game_request_handler_test.cpp` |
| Change client-side click-to-select-then-move UX | `client/src/game/GameClient.cpp` (`handlePixelClick`) |
| Parse/print board text (REPL only, not production setup) | `server/src/game/IO/BoardParser.cpp` / `BoardPrinter.cpp` |
| Change the initial/starting board setup | `server/src/game/IO/GameFactory.cpp` (`createClassicBoard`) |
| Change the live loop, mouse handling, or when the game stops | `client/src/ui/GameLoop.cpp` |
| Change the per-frame data the UI renders from | `server/src/game/Controller/Controller.cpp` (`getGameView`), `common/DTO/GameView.h`/`MotionView.h`/`JumpView.h`/`RestView.h` |
| Change the traveling-piece animation, jumping-piece animation, resting-piece animation, selection/jump highlights, or rest-progress bar | `client/src/ui/AnimationFrame.cpp`, `client/src/ui/Renderer.cpp` (draw order), `client/src/ui/BoardCanvas.cpp` (pixel math, colors) |
| Fix/extend sprite rendering | `common/enums/PieceStateToString.h`, `client/src/ui/SpriteManager.cpp`, `client/src/ui/AnimationFrame.cpp` |
| Change how pixel clicks map to board cells | `client/src/game/BoardMapper.cpp`, fed via `GameClient::handlePixelClick`/`handlePixelJump` |
| Add/adjust server-side domain tests | `server/tests/game_tests/<matching-folder>/`, `server/tests/common_tests/` |
| Add a new event / subscribe to a game event | `common/EventBus/Events.h` (new event struct), publisher call sites in `server/src/game/Engine/GameEngine.cpp`, subscriber wiring in `server/src/services/GameSession/GameSession.cpp` (`subscribeToEvents`) |
| Change the `users` table / user persistence | `server/src/persistence/Sqlite/SqliteDatabase.cpp` (SQLite schema), `server/src/persistence/Sqlite/SqliteUserRepository.cpp` (SQLite impl), `server/src/persistence/Postgres/PostgresDatabase.cpp` (Postgres schema), `server/src/persistence/Postgres/PostgresUserRepository.cpp` (Postgres impl), `server/src/persistence/InMemory/InMemoryUserRepository.cpp` (in-memory impl); tests in `server/tests/sqlite_user_repository_test.cpp`/`postgres_user_repository_test.cpp`/`in_memory_user_repository_test.cpp` |
| Add a new user-persistence backend | `server/src/persistence/IUserRepository.h` (implement the interface), `server/src/persistence/Factory/RepositoryFactory.cpp` (add a `RepositoryBackend` value + `case`) |
| Switch which backend `KungFuChessWsGateway`/`KungFuChessGameNode`/`KungFuChessApiGateway` actually opens | `server/src/main.cpp`/`gamenode/src/main.cpp`/`apigateway/src/main.cpp` (each currently `RepositoryBackend::Sqlite` unless `KUNGFUCHESS_POSTGRES_URL` is set — **must be set the same way on all three, see Known gaps #19**; `gameallocator/src/main.cpp` needs no user-repository backend at all, see its own review note) |
| Change password hashing (cost factor, algorithm) | `server/src/security/SecurityConfig.h`, `server/src/security/PasswordHasher.cpp`; tests in `server/tests/password_hasher_test.cpp` |
| Change register/login business logic (not the DB rows) | `server/src/services/Auth/AuthService.cpp` (called only from `apigateway/src/ApiGatewayRequestHandler.cpp` now — **Phase 2**, `server/`'s own `AuthRequestHandler` never calls it); tests in `server/tests/auth_service_test.cpp`, `apigateway/tests/api_gateway_request_handler_test.cpp` |
| Add/change a REST endpoint on the API Gateway | `apigateway/src/ApiGatewayRequestHandler.cpp` (request parsing/business logic, testable without HTTP), `apigateway/src/network/ApiGatewayServer.cpp` (path/method routing) |
| Change how a login token is issued/verified, its TTL, or its signing secret | `common/Security/TokenService.cpp` (issue/verify logic), `common/Security/Sha256.cpp` (the HMAC primitive), `common/Config/TokenConfig.h` (TTL, dev-default secret), `server/src/main.cpp`/`apigateway/src/main.cpp` (`KUNGFUCHESS_TOKEN_SECRET` env wiring — **must match on both, see Known gaps #18**); tests in `server/tests/common_tests/token_service_test.cpp` |
| Change game-session hosting, tick behavior, or session lookup | `server/src/services/GameSession/GameSession.cpp`/`GameSessionManager.cpp` (unchanged in content — constructed by `gamenode/src/main.cpp` now, not `server/src/main.cpp`); tests in `server/tests/game_session_test.cpp` |
| Change matchmaking pairing rules, wait timeout, or score range | `common/Config/MatchmakingConfig.h`, `server/src/services/Matchmaking/Matchmaker.cpp` (constructed by `gameallocator/src/main.cpp` now — **Phase 4b**, moved from `gamenode/`); tests in `server/tests/matchmaker_test.cpp` |
| Change connection-identity/matchmaking-queue/session-index storage, or add a new backend for one | `server/src/services/Connection/IConnectionStore.h`/`Matchmaking/IMatchQueueStore.h`/`SessionIndex/ISessionIndexStore.h` (implement the interface), their `Local*`/`Redis*` implementations; tests in `server/tests/local_connection_store_test.cpp`/`redis_connection_store_test.cpp`/`redis_match_queue_store_test.cpp`/`redis_session_index_store_test.cpp` |
| Switch `ConnectionRegistry`/`Matchmaker`/`GameSessionManager` to Redis-backed storage | `server/src/main.cpp` (`KungFuChessWsGateway`), `gameallocator/src/main.cpp` (`KungFuChessGameAllocator` — `ConnectionRegistry`/`Matchmaker`), and `gamenode/src/main.cpp` (`KungFuChessGameNode` — `GameSessionManager`) — **Phase 3: `KUNGFUCHESS_REDIS_URL` is mandatory on all of them now, not opt-in, see Known gaps #21** |
| Change disconnect-grace duration | `common/Config/MatchmakingConfig.h`, `server/src/services/GameSession/GameSession.cpp` (`tick`, `forfeitTo`) |
| Change ELO rating math (K-factor, starting rating) or how a game outcome is persisted | `common/Config/RatingConfig.h`, `server/src/services/Rating/EloCalculator.cpp` (math), `server/src/services/Rating/RatingService.cpp` (persistence); tests in `server/tests/elo_calculator_test.cpp`/`rating_service_test.cpp` |
| Change reconnect detection/resume behavior | `server/src/handlers/AuthRequestHandler.cpp` (`completeLogin`, called from the `login_token` branch), `server/src/services/GameNodeBridge/IReconnectResolver.h`/`LocalReconnectResolver.cpp`/`RemoteReconnectResolver.cpp` (**Phase 3**: the seam between them; **Phase 4b**: `RemoteReconnectResolver` now resolves a shard via `IGameShardRoutingStore` first), `server/src/services/GameSession/GameSessionManager.cpp` (`rebindConnection`), `server/src/services/GameSession/GameSession.cpp` (`markReconnected`, `resumeInfoFor`); tests in `server/tests/local_reconnect_resolver_test.cpp`/`auth_request_handler_test.cpp` |
| Change the Gateway↔Allocator↔Shard Redis channel names or the reconnect-check timeout | `common/Config/GameNodeConfig.h` (`MATCHMAKING_REQUESTS_CHANNEL`, `shardRequestsChannel(shardId)`, `PUSHES_CHANNEL`, `RECONNECT_CHECK_TIMEOUT_MILLIS`) |
| Add a new kind of Gateway/Allocator/Shard message, or change how one is dispatched | `server/src/services/GameNodeBridge/GameNodeMessages.h` (`GameNodeRequest`/`GameNodeCreateSessionRequest`/`GameNodePush` envelope shapes), `GameNodeRequestPublisher.cpp`/`GameNodePushRouter.cpp`/`GatewayGameRouter.cpp` (Gateway side), `GameAllocatorRequestRouter.cpp` (Allocator side), `GameNodeRequestRouter.cpp`/`GameNodePushPublisher.cpp` (Shard side); tests in `server/tests/game_node_push_router_test.cpp` |
| Change what `KungFuChessGameAllocator`/`KungFuChessGameNode` constructs/wires at startup | `gameallocator/src/main.cpp` (Matchmaker/GameAllocator side) / `gamenode/src/main.cpp` (GameSessionManager side) |
| Change shard-picking logic, or which shard a session/user routes to | `server/src/services/Sharding/GameAllocator.cpp` (`pickShard`/`allocate`), `IShardLoadStore.h`/`IGameShardRoutingStore.h` (implement a new backend); tests in `server/tests/game_allocator_test.cpp` |
| Change how a Game Server Shard identifies itself, or what happens to shard-load/routing state when a session ends normally | `gamenode/src/main.cpp` (`KUNGFUCHESS_SHARD_ID` wiring, `GameSessionManager`'s `SessionFinishedFn` callback), `server/src/services/GameSession/GameSessionManager.cpp` (`removeFinishedSessions`); tests in `server/tests/game_session_test.cpp` |
| Change how a move/jump/find_game request is routed to the Allocator vs. a specific shard | `server/src/services/GameNodeBridge/GatewayGameRouter.cpp` (`handle`, `shardChannelForConnection`) |
| Change shard heartbeat/TTL timing, or what happens when a shard is presumed dead | `common/Config/GameNodeConfig.h` (`SHARD_HEARTBEAT_INTERVAL_MILLIS`/`SHARD_HEARTBEAT_TTL_MILLIS`), `gamenode/src/main.cpp` (`runTickLoop`'s heartbeat call), `server/src/services/Sharding/ShardHealthMonitor.cpp` (`checkAndForfeitDeadShards`/`forfeitShard`), `IShardLoadStore.h` (`heartbeat`/`isAlive`/`forget`, implement a new backend); tests in `server/tests/shard_health_monitor_test.cpp`, `server/tests/redis_shard_load_store_test.cpp` |
| Change what happens to a shard-crash forfeit (rating, message shape) | `server/src/services/Sharding/ShardHealthMonitor.cpp` (`forfeitShard` — currently pushes `GameOverMessage{"shard_unavailable", 0}`, no rating change; see Known gaps #26) |
| Change the auth/matchmaking/game/CLI/GUI hand-off order on the client | `client/src/main.cpp` |
| Change the REST `register`/`login` calls the client makes, or add a new API Gateway endpoint client-side | `client/src/network/ApiGatewayClient.cpp`, `client/src/cli/CliShell.cpp` |
| Change the JSON wire format for a message | `protocol/include/protocol/Message.h` (field names, `toJson`/`fromJson`), `protocol/include/protocol/MessageType.h` (the `"type"` string); round-trip tests in `protocol/tests/message_test.cpp` |
| Add a DTO's own `toJson`/`fromJson`, or change one | `common/DTO/<Type>.h`/`.cpp` — but keep any converting-from-domain constructor in `<Type>FromDomain.cpp` (see DTO layer / Gaps #12 above) |
| Add a new WebSocket message type/command | If it needs auth (like `login_token`): `server/src/handlers/AuthRequestHandler.cpp`. Otherwise (anything matchmaking/game-shaped): `server/src/handlers/GameRequestHandler.cpp` (`gamenode/`) or `MatchmakingRequestHandler.cpp` (`gameallocator/`) — **Phase 4b**: `GatewayGameRouter::handle` (`server/`) now needs to know its `"type"` explicitly to route it to the right channel (unlike Phase 3's `RemoteGameNodeHandler`, which forwarded anything auth didn't recognize unconditionally to a single Game Node). Either way: `protocol/include/protocol/Message.h`/`MessageType.h` for the new request/result structs, `client/src/game/GameClient.cpp` or `client/src/cli/CliShell.cpp` for the client side — `WebSocketServer`/`WebSocketClient` themselves don't need to change for a new message type |
| Change how the server binds/accepts WebSocket connections, sends targeted/broadcast pushes, or reports drops | `server/src/network/WebSocketServer.cpp` (the only file including ixwebsocket's server headers) |
| Change how the client connects or sends/receives | `client/src/network/WebSocketClient.cpp` (the only file including ixwebsocket's client headers) |
| Change the health-check endpoint (port, response body) | `server/src/network/HealthCheckServer.cpp`, `common/Config/NetworkConfig.h` (`HEALTH_CHECK_PORT`) |
| Change log format/level or add a new log call | `common/Logging/Logger.h`/`.cpp` (`common::Logger::debug`/`info`/`warn`/`error`) |
| Change the Docker image or what's built for it | `Dockerfile`, root `CMakeLists.txt` (`BUILD_CLIENT` option) |
| Change what's in the local Docker Compose stack | `docker-compose.yml` |
| Change the interface a process binds (native default vs. inside Docker) | `server/src/main.cpp`/`gameallocator/src/main.cpp`/`gamenode/src/main.cpp` (all read `KUNGFUCHESS_HOST`), `server/src/network/WebSocketServer.h`/`.cpp` (`host` constructor parameter, used by `server/src/main.cpp` only) |
