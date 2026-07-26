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
target_link_libraries(KungFuChessCliClient PRIVATE KungFuChessClientNetwork KungFuChessCommon)

# ---- Client-side game glue (BoardMapper + GameClient) ----
# The seam GameLoop talks to instead of a same-process Controller& -- moved
# from src/logic/Controller/BoardMapper (BoardMapper) and new (GameClient).
file(GLOB CLIENT_GAME_SOURCES "src/game/*.cpp")
file(GLOB CLIENT_GAME_HEADERS "src/game/*.h")

add_library(KungFuChessClientGame STATIC ${CLIENT_GAME_SOURCES} ${CLIENT_GAME_HEADERS})
target_include_directories(KungFuChessClientGame PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(KungFuChessClientGame PUBLIC KungFuChessClientNetwork KungFuChessCommon)

# ---- UI/rendering layer (moved here from src/UI/) ----
file(GLOB CLIENT_UI_SOURCES "src/ui/*.cpp")
file(GLOB CLIENT_UI_HEADERS "src/ui/*.h")

add_library(KungFuChessClientUi STATIC ${CLIENT_UI_SOURCES} ${CLIENT_UI_HEADERS})
target_include_directories(KungFuChessClientUi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src ${OPENCV_INCLUDE_DIR})
target_link_directories(KungFuChessClientUi PUBLIC ${OPENCV_LIB_DIR})
target_link_libraries(KungFuChessClientUi PUBLIC
    KungFuChessClientGame
    KungFuChessCommon
    $<$<CONFIG:Debug>:${OPENCV_LIB_DIR}/opencv_world451d.lib>
    $<$<CONFIG:Release>:${OPENCV_LIB_DIR}/opencv_world451.lib>
    $<$<CONFIG:RelWithDebInfo>:${OPENCV_LIB_DIR}/opencv_world451.lib>
    $<$<CONFIG:MinSizeRel>:${OPENCV_LIB_DIR}/opencv_world451.lib>
)

# ---- GUI executable (the actual networked replacement for the retired
# local-hotseat KungFuChess executable) ----
add_executable(KungFuChessGuiClient src/gui_main.cpp)
target_include_directories(KungFuChessGuiClient PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(KungFuChessGuiClient PRIVATE
    KungFuChessClientUi
    KungFuChessClientGame
    KungFuChessClientNetwork
    KungFuChessCommon)

if(WIN32)
    add_custom_command(TARGET KungFuChessGuiClient POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<$<CONFIG:Debug>:${OPENCV_LIB_DIR}/opencv_world451d.dll>
        $<$<CONFIG:Release>:${OPENCV_LIB_DIR}/opencv_world451.dll>
        $<$<CONFIG:RelWithDebInfo>:${OPENCV_LIB_DIR}/opencv_world451.dll>
        $<$<CONFIG:MinSizeRel>:${OPENCV_LIB_DIR}/opencv_world451.dll>
        $<TARGET_FILE_DIR:KungFuChessGuiClient>)
endif()

# ---- Tests (BoardMapper -- pure pixel/cell math, no network/rendering I/O
# involved, so no OpenCV/ixwebsocket needed to exercise it) ----
file(GLOB CLIENT_TEST_SOURCES "tests/*.cpp")

add_executable(KungFuChessClient_tests ${CLIENT_TEST_SOURCES})
target_include_directories(KungFuChessClient_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(KungFuChessClient_tests PRIVATE KungFuChessClientGame)
