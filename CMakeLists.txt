cmake_minimum_required(VERSION 3.16)
project(KungFuChess)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# OpenCV location (vendored under lib/). Kept at root scope since client/ will
# link this for its GUI executable once UI/ is relocated there.
set(OPENCV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(OPENCV_INCLUDE_DIR "${OPENCV_DIR}/include")
set(OPENCV_LIB_DIR "${OPENCV_DIR}/bin")

# ---- Shared networking/protocol dependencies ----
# Declared once here (not inside server/ or client/) because both subdirectories
# need them -- FetchContent_MakeAvailable for the same package from two different
# subdirectories would redefine its targets.
include(FetchContent)

set(BUILD_DEMO OFF CACHE BOOL "" FORCE)
set(USE_WS OFF CACHE BOOL "" FORCE)
set(USE_TEST OFF CACHE BOOL "" FORCE)
set(USE_TLS OFF CACHE BOOL "" FORCE)   # plain ws:// only, no wss://
set(USE_ZLIB OFF CACHE BOOL "" FORCE)  # no permessage-deflate needed for small JSON messages
set(IXWEBSOCKET_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    ixwebsocket
    GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
    GIT_TAG        v12.0.1
)
FetchContent_MakeAvailable(ixwebsocket)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.12.0
)
FetchContent_MakeAvailable(nlohmann_json)

# ---- Shared common library (DTOs/enums/EventBus/config, used by server/ and client/) ----
add_subdirectory(common)

# ---- Shared protocol (JSON message contract used by server/ and client/) ----
add_subdirectory(protocol)

# ---- Server (persistence + game engine + WebSocket networking layer) ----
add_subdirectory(server)

# ---- CLI client (standalone executable, talks to the server over ws://) ----
add_subdirectory(client)
