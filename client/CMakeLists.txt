# KungFuChess CLI client: a standalone executable that talks to
# KungFuChessServer over ws://, as a text-based stand-in for the GUI client
# during early development. ixwebsocket/nlohmann_json/KungFuChessProtocol are
# fetched/defined once in the root CMakeLists.txt (server/ needs them too).

# ---- Client network layer (the piece a future GUI client reuses as-is) ----
file(GLOB CLIENT_NETWORK_SOURCES "src/network/*.cpp")
file(GLOB CLIENT_NETWORK_HEADERS "src/network/*.h")

add_library(KungFuChessClientNetwork STATIC ${CLIENT_NETWORK_SOURCES} ${CLIENT_NETWORK_HEADERS})
target_include_directories(KungFuChessClientNetwork PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(KungFuChessClientNetwork PUBLIC KungFuChessProtocol ixwebsocket::ixwebsocket)

# ---- CLI executable ----
file(GLOB CLI_SOURCES "src/cli/*.cpp")
file(GLOB CLI_HEADERS "src/cli/*.h")

add_executable(KungFuChessCliClient src/main.cpp ${CLI_SOURCES} ${CLI_HEADERS})
target_include_directories(KungFuChessCliClient PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(KungFuChessCliClient PRIVATE KungFuChessClientNetwork)
