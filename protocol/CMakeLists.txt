# Header-only JSON message contract shared by server/ and client/ (and, later,
# the GUI client) so every consumer builds requests/responses from one
# definition instead of duplicating field names on both ends of the socket.

add_library(KungFuChessProtocol INTERFACE)
target_include_directories(KungFuChessProtocol INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
# Message.h wraps common::GameView (the game-session snapshot DTO) directly,
# so this now depends on KungFuChessCommon too, not just nlohmann_json.
target_link_libraries(KungFuChessProtocol INTERFACE nlohmann_json::nlohmann_json KungFuChessCommon)

# ---- Tests (message round-trip serialization) ----
# Deliberately does NOT link KungFuChessGame: these tests only ever construct
# common::DTOs via their plain-field constructors and fromJson() (never the
# converting-from-domain-object constructors like PositionView(const
# Position&)), and each such converting constructor now lives in its own
# translation unit (see common/DTO/*FromDomain.cpp) precisely so consumers
# like this one never need the server's domain model linked in at all.
file(GLOB PROTOCOL_TEST_SOURCES "tests/*.cpp")

add_executable(KungFuChessProtocol_tests ${PROTOCOL_TEST_SOURCES})
target_include_directories(KungFuChessProtocol_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(KungFuChessProtocol_tests PRIVATE KungFuChessProtocol)
