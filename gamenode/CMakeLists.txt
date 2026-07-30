# Game Node / Game Server Shard (MIGRATION_PLAN.md Phase 3): the process
# that owns one shard's GameSessionManager/GameSession's tick loop -- moved
# out of server/'s KungFuChessServer executable, but not out of server/'s
# source tree, since these classes are unchanged in content (see
# server/src/services/GameSession, server/src/handlers/GameRequestHandler)
# and already compiled into KungFuChessNetwork/KungFuChessGameSession there.
# This mirrors apigateway/'s precedent exactly: reuse a server/ library
# directly instead of duplicating or relocating its logic. Phase 4b narrowed
# this module's scope: Matchmaker/MatchmakingRequestHandler/GameAllocator
# moved out to their own singleton process (see ../gameallocator/), since
# they need to exist exactly once no matter how many of *this* process run
# at once. The only new code in this whole module is src/main.cpp itself,
# which wires GameSessionManager/GameRequestHandler to
# GameNodeRequestRouter/GameNodePushPublisher (server/src/services/
# GameNodeBridge/) instead of to a WebSocketServer it never holds.

add_executable(KungFuChessGameNode src/main.cpp)
target_link_libraries(KungFuChessGameNode PRIVATE KungFuChessNetwork KungFuChessCommon)
