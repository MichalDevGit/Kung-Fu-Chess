# KungFuChess server: persistence + WebSocket networking layer.
# ixwebsocket/nlohmann_json/KungFuChessProtocol are fetched/defined once in the
# root CMakeLists.txt (client/ needs them too) -- only SQLiteCpp is fetched here,
# since it's server-only.

include(FetchContent)

# Disabled: SQLiteCpp's own cpplint.py step rejects the CRLF line endings its
# vendored sources are checked out with on Windows, breaking the default
# ALL_BUILD target for reasons that have nothing to do with this project's code.
set(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "" FORCE)
set(SQLITECPP_RUN_CPPCHECK OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    SQLiteCpp
    GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
    GIT_TAG        3.3.1
    GIT_SUBMODULES ""
)
FetchContent_MakeAvailable(SQLiteCpp)

# ---- Persistence library ----
file(GLOB PERSISTENCE_SOURCES "src/persistence/*.cpp")
file(GLOB PERSISTENCE_HEADERS "src/persistence/*.h")

add_library(KungFuChessPersistence STATIC ${PERSISTENCE_SOURCES} ${PERSISTENCE_HEADERS})
target_include_directories(KungFuChessPersistence PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(KungFuChessPersistence PUBLIC SQLiteCpp)

# ---- Auth service (DB-backed business logic; no networking types) ----
file(GLOB AUTH_SOURCES "src/services/AuthService.cpp")
file(GLOB AUTH_HEADERS "src/services/AuthService.h")

add_library(KungFuChessAuth STATIC ${AUTH_SOURCES} ${AUTH_HEADERS})
target_include_directories(KungFuChessAuth PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(KungFuChessAuth PUBLIC KungFuChessPersistence)

# ---- Game engine (authoritative domain logic: model/rules/Engine/Controller/IO) ----
# Moved here from src/logic/* -- this is now the single source of truth for
# game rules/state, with the server as its only host (the UI/client never
# links this directly; it only ever sees serialized GameView JSON).
file(GLOB_RECURSE GAME_SOURCES "src/game/*.cpp")
file(GLOB_RECURSE GAME_HEADERS "src/game/*.h")

add_library(KungFuChessGame STATIC ${GAME_SOURCES} ${GAME_HEADERS})
target_include_directories(KungFuChessGame PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(KungFuChessGame PUBLIC KungFuChessCommon)

# ---- Game session service (hosts one authoritative GameEngine/Controller
# per session, bridges its EventBus events to protocol/ messages) ----
# Kept as its own target, separate from KungFuChessAuth, even though both
# live under src/services/ -- a library named "Auth" building GameSession
# would be a misleading name for what it actually contains.
file(GLOB GAME_SESSION_SOURCES "src/services/GameSession.cpp" "src/services/GameSessionManager.cpp")
file(GLOB GAME_SESSION_HEADERS "src/services/GameSession.h" "src/services/GameSessionManager.h")

add_library(KungFuChessGameSession STATIC ${GAME_SESSION_SOURCES} ${GAME_SESSION_HEADERS})
target_include_directories(KungFuChessGameSession PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
# KungFuChessPersistence added for UserRepository -- GameSessionManager takes
# one directly so it can hand every GameSession it creates a ScoreUpdateFn
# closing over it (see GameSessionManager::createSession), for forfeit-by-
# disconnect score deltas.
target_link_libraries(KungFuChessGameSession PUBLIC KungFuChessGame KungFuChessProtocol KungFuChessPersistence)

# ---- Network layer (request handlers + the WebSocket server wrapper) ----
# ConnectionRegistry/Matchmaker live under src/services/ alongside GameSession
# (they're services in the same sense), but are compiled into this library
# specifically -- neither depends on KungFuChessGame/KungFuChessGameSession at
# all, and the handlers that do need them (AuthRequestHandler,
# MatchmakingRequestHandler) already live here.
file(GLOB NETWORK_SOURCES "src/handlers/*.cpp" "src/network/*.cpp"
    "src/services/ConnectionRegistry.cpp" "src/services/Matchmaker.cpp")
file(GLOB NETWORK_HEADERS "src/handlers/*.h" "src/network/*.h"
    "src/services/ConnectionRegistry.h" "src/services/Matchmaker.h")

add_library(KungFuChessNetwork STATIC ${NETWORK_SOURCES} ${NETWORK_HEADERS})
target_include_directories(KungFuChessNetwork PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(KungFuChessNetwork PUBLIC KungFuChessAuth KungFuChessGameSession KungFuChessProtocol ixwebsocket::ixwebsocket)

# ---- Tests (persistence + auth service + game engine; no sockets involved) ----
# GLOB_RECURSE so this also picks up tests/game_tests/**/*.cpp (moved from
# src/tests/logic_tests -- see game/ above). "tests/doctest.h" resolves via
# ${CMAKE_CURRENT_SOURCE_DIR} below (server/tests/doctest.h, moved from
# src/tests/doctest.h); "game/X/Y.h" resolves via KungFuChessGame's PUBLIC
# include dir, same pattern already used for "persistence/..."/"services/...".
file(GLOB_RECURSE PERSISTENCE_TEST_SOURCES "tests/*.cpp")

add_executable(KungFuChessPersistence_tests ${PERSISTENCE_TEST_SOURCES})
target_include_directories(KungFuChessPersistence_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(KungFuChessPersistence_tests PRIVATE KungFuChessPersistence KungFuChessAuth KungFuChessGame KungFuChessGameSession KungFuChessNetwork)

# ---- Server executable (serves auth over ws://127.0.0.1:9002) ----
add_executable(KungFuChessServer src/main.cpp)
target_link_libraries(KungFuChessServer PRIVATE KungFuChessNetwork KungFuChessCommon)
