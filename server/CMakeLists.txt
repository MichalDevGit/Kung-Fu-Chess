# KungFuChess server: persistence + WebSocket networking layer.
# ixwebsocket/nlohmann_json/KungFuChessProtocol are fetched/defined once in the
# root CMakeLists.txt (client/ needs them too) -- only SQLiteCpp is fetched here,
# since it's server-only.

include(FetchContent)

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
file(GLOB AUTH_SOURCES "src/services/*.cpp")
file(GLOB AUTH_HEADERS "src/services/*.h")

add_library(KungFuChessAuth STATIC ${AUTH_SOURCES} ${AUTH_HEADERS})
target_include_directories(KungFuChessAuth PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(KungFuChessAuth PUBLIC KungFuChessPersistence)

# ---- Network layer (request handlers + the WebSocket server wrapper) ----
file(GLOB NETWORK_SOURCES "src/handlers/*.cpp" "src/network/*.cpp")
file(GLOB NETWORK_HEADERS "src/handlers/*.h" "src/network/*.h")

add_library(KungFuChessNetwork STATIC ${NETWORK_SOURCES} ${NETWORK_HEADERS})
target_include_directories(KungFuChessNetwork PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(KungFuChessNetwork PUBLIC KungFuChessAuth KungFuChessProtocol ixwebsocket::ixwebsocket)

# ---- Tests (persistence + auth service; no sockets involved) ----
file(GLOB PERSISTENCE_TEST_SOURCES "tests/*.cpp")

add_executable(KungFuChessPersistence_tests ${PERSISTENCE_TEST_SOURCES})
target_include_directories(KungFuChessPersistence_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(KungFuChessPersistence_tests PRIVATE KungFuChessPersistence KungFuChessAuth)

# ---- Server executable (serves auth over ws://127.0.0.1:9002) ----
add_executable(KungFuChessServer src/main.cpp)
target_link_libraries(KungFuChessServer PRIVATE KungFuChessNetwork)
