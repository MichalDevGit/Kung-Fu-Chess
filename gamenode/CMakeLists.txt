# Game Node (MIGRATION_PLAN.md Phase 3): the process that now actually owns
# GameSessionManager/Matchmaker/GameSession's tick loop -- moved out of
# server/'s KungFuChessServer executable, but not out of server/'s source
# tree, since these classes are unchanged in content (see
# server/src/services/GameSession, server/src/services/Matchmaking,
# server/src/handlers/{Matchmaking,Game}RequestHandler) and already compiled
# into KungFuChessNetwork/KungFuChessGameSession there. This mirrors
# apigateway/'s precedent exactly: reuse a server/ library directly instead
# of duplicating or relocating its logic. The only new code in this whole
# module is src/main.cpp itself, which wires those existing classes to
# GameNodeRequestRouter/GameNodePushPublisher (server/src/services/
# GameNodeBridge/) instead of to a WebSocketServer it no longer holds.

add_executable(KungFuChessGameNode src/main.cpp)
target_link_libraries(KungFuChessGameNode PRIVATE KungFuChessNetwork KungFuChessCommon)
