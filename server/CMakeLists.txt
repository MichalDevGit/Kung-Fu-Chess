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

# ---- Postgres (Phase 1 of MIGRATION_PLAN.md: PostgresUserRepository) ----
# Optional, unlike SQLiteCpp/bcrypt above: libpqxx links against libpq (the
# official Postgres C client), and libpq's own build (autoconf/make, needs
# bison/flex/perl) isn't something this project can vendor-and-build-from-
# source under MSVC the way bcrypt was hand-rolled. QUIET here (not REQUIRED)
# so a native Windows configure with no system Postgres install keeps working
# exactly as before -- PostgresUserRepository.cpp/.h simply compile to empty
# translation units, and RepositoryFactory throws a clear error if
# RepositoryBackend::Postgres is selected in a build where this wasn't found.
# The Docker/Linux build always finds it (see Dockerfile's libpq-dev).
find_package(PostgreSQL QUIET)
if(PostgreSQL_FOUND)
    set(SKIP_BUILD_TEST ON CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        libpqxx
        GIT_REPOSITORY https://github.com/jtv/libpqxx.git
        GIT_TAG        7.9.2
    )
    FetchContent_MakeAvailable(libpqxx)
endif()

# bcrypt (vendored source only -- its own CMakeLists.txt targets GCC/make
# with a GNU-assembler .S file and won't configure under the MSVC generator
# this project builds with, so FetchContent_Populate fetches the sources
# without ever running that CMakeLists.txt; the bcrypt_vendor target below is
# our own, hand-written to build cleanly on MSVC). Pinned by commit since this
# fork has no tags. x86.S is deliberately excluded from bcrypt_vendor's
# sources: crypt_blowfish.c only takes its asm path under "#ifdef __i386__",
# a GCC-only macro MSVC never defines, so the portable C path is always used
# here regardless.
FetchContent_Declare(
    bcrypt
    GIT_REPOSITORY https://github.com/ZoneMinder/libbcrypt.git
    GIT_TAG        be171cd75dd65e06315a67c7dcdb8e1bbc1dabd4
)
FetchContent_GetProperties(bcrypt)
if(NOT bcrypt_POPULATED)
    FetchContent_Populate(bcrypt)
endif()

add_library(bcrypt_vendor STATIC
    ${bcrypt_SOURCE_DIR}/src/bcrypt.c
    ${bcrypt_SOURCE_DIR}/src/crypt_blowfish.c
    ${bcrypt_SOURCE_DIR}/src/crypt_gensalt.c
    ${bcrypt_SOURCE_DIR}/src/wrapper.c
)
target_include_directories(bcrypt_vendor PUBLIC ${bcrypt_SOURCE_DIR}/include)
if(WIN32)
    # bcrypt_gensalt's Windows path uses CryptAcquireContext/CryptGenRandom
    # (wincrypt.h) for salt randomness instead of /dev/urandom.
    target_link_libraries(bcrypt_vendor PUBLIC Advapi32)
else()
    # bcrypt.c/crypt_blowfish.c/crypt_gensalt.c/wrapper.c's non-_WIN32 branch
    # bare-includes their own headers (e.g. #include "bcrypt.h", not
    # "bcrypt/bcrypt.h") -- needs the headers' own directory on the include
    # path directly, not just its parent, to compile on Linux/GCC. Must stay
    # WIN32-gated: on Windows this directory on the include path makes
    # #include <bcrypt.h> (pulled in transitively via wincrypt.h/ncrypt.h)
    # resolve to this vendored header instead of the real Windows CNG
    # bcrypt.h, breaking the MSVC build entirely.
    target_include_directories(bcrypt_vendor PRIVATE ${bcrypt_SOURCE_DIR}/include/bcrypt)
endif()

# ---- Security library (password hashing -- the only thing that gets to
# know bcrypt exists at all; see PasswordHasher.h) ----
file(GLOB SECURITY_SOURCES "src/security/*.cpp")
file(GLOB SECURITY_HEADERS "src/security/*.h")

add_library(KungFuChessSecurity STATIC ${SECURITY_SOURCES} ${SECURITY_HEADERS})
target_include_directories(KungFuChessSecurity PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(KungFuChessSecurity PUBLIC bcrypt_vendor)

# ---- Persistence library ----
file(GLOB PERSISTENCE_SOURCES "src/persistence/*.cpp")
file(GLOB PERSISTENCE_HEADERS "src/persistence/*.h")

add_library(KungFuChessPersistence STATIC ${PERSISTENCE_SOURCES} ${PERSISTENCE_HEADERS})
target_include_directories(KungFuChessPersistence PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(KungFuChessPersistence PUBLIC SQLiteCpp)
if(PostgreSQL_FOUND)
    # PUBLIC: RepositoryFactory.cpp (a different translation unit in this
    # same target) also needs KUNGFUCHESS_HAS_POSTGRES defined to know to
    # #include PostgresUserRepository.h and compile its real Postgres case.
    target_link_libraries(KungFuChessPersistence PUBLIC pqxx)
    target_compile_definitions(KungFuChessPersistence PUBLIC KUNGFUCHESS_HAS_POSTGRES)
endif()

# ---- Auth service (DB-backed business logic; no networking types) ----
file(GLOB AUTH_SOURCES "src/services/AuthService.cpp")
file(GLOB AUTH_HEADERS "src/services/AuthService.h")

add_library(KungFuChessAuth STATIC ${AUTH_SOURCES} ${AUTH_HEADERS})
target_include_directories(KungFuChessAuth PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(KungFuChessAuth PUBLIC KungFuChessPersistence KungFuChessSecurity)

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
file(GLOB GAME_SESSION_SOURCES "src/services/GameSession.cpp" "src/services/GameSessionManager.cpp"
    "src/services/EloCalculator.cpp" "src/services/RatingService.cpp")
file(GLOB GAME_SESSION_HEADERS "src/services/GameSession.h" "src/services/GameSessionManager.h"
    "src/services/EloCalculator.h" "src/services/RatingService.h")

add_library(KungFuChessGameSession STATIC ${GAME_SESSION_SOURCES} ${GAME_SESSION_HEADERS})
target_include_directories(KungFuChessGameSession PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
# KungFuChessPersistence added for IUserRepository -- GameSessionManager owns a
# RatingService built from one, and hands every GameSession it creates a
# GameOutcomeFn closing over it (see GameSessionManager::createSession), for
# both game-ending paths' ELO rating updates (GameSession::forfeitTo and its
# GameOverEvent subscriber).
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
