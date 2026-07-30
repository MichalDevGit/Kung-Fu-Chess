# Game Allocator (MIGRATION_PLAN.md Phase 4b): the one process that owns
# Matchmaker/MatchmakingRequestHandler/GameAllocator -- split out of
# gamenode/ (which used to own these alongside GameSessionManager, back when
# Phase 3 only ever ran one Game Node so there was no distinction between
# "the matchmaking singleton" and "a shard"). Not out of server/'s source
# tree, though: these classes are unchanged in content and already compiled
# into KungFuChessNetwork/KungFuChessGameSession there, exactly the same
# "reuse a server/ library directly, don't relocate or duplicate its logic"
# precedent apigateway/ and gamenode/ both already established. The only new
# code in this whole module is src/main.cpp itself, which wires those
# existing classes to GameAllocatorRequestRouter/GameNodePushPublisher/
# GameNodeRequestPublisher (server/src/services/GameNodeBridge/) instead of
# to a WebSocketServer it never holds.

add_executable(KungFuChessGameAllocator src/main.cpp)
target_link_libraries(KungFuChessGameAllocator PRIVATE KungFuChessNetwork KungFuChessCommon)
