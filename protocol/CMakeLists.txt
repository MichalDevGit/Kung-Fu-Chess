# Header-only JSON message contract shared by server/ and client/ (and, later,
# the GUI client) so every consumer builds requests/responses from one
# definition instead of duplicating field names on both ends of the socket.

add_library(KungFuChessProtocol INTERFACE)
target_include_directories(KungFuChessProtocol INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(KungFuChessProtocol INTERFACE nlohmann_json::nlohmann_json)
