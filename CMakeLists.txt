cmake_minimum_required(VERSION 3.16)
project(KungFuChess)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find all source files in src directory
file(GLOB_RECURSE ALL_SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.hpp" "src/*.h")

# Split sources: everything under src/tests/ belongs to the test executable,
# the rest (logic + UI + main.cpp) belongs to the game executable.
set(TEST_SOURCES ${ALL_SOURCES})
list(FILTER TEST_SOURCES INCLUDE REGEX "src/tests/")
# test_main.cpp is the doctest runner (its DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
# provides main()). src/tests/main.cpp is a separate console REPL entry point with
# its own main(), so exclude it here to avoid two main() symbols in the test target.
list(FILTER TEST_SOURCES EXCLUDE REGEX "src/tests/main.cpp")

set(GAME_SOURCES ${ALL_SOURCES})
list(FILTER GAME_SOURCES EXCLUDE REGEX "src/tests/")

# The shared logic/UI sources (no main.cpp) are reused by both executables.
set(LIB_SOURCES ${GAME_SOURCES})
list(FILTER LIB_SOURCES EXCLUDE REGEX "src/main.cpp")

# ---- Game executable ----
add_executable(${PROJECT_NAME} ${GAME_SOURCES} ${HEADERS})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# ---- Test executable ----
# Tests reuse the shared logic/UI sources plus their own test files.
# They include headers relative to src/ (e.g. "tests/doctest.h"), so src is on the include path.
set(TEST_TARGET ${PROJECT_NAME}_tests)
add_executable(${TEST_TARGET} ${TEST_SOURCES} ${LIB_SOURCES})
# Test files include headers two ways: "tests/doctest.h" (relative to src/)
# and "src/logic/.../X.h" (relative to the project root), so both are on the path.
target_include_directories(${TEST_TARGET} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Set OpenCV paths
set(OPENCV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(OPENCV_INCLUDE_DIR "${OPENCV_DIR}/include")
set(OPENCV_LIB_DIR "${OPENCV_DIR}/bin")

# Apply OpenCV include dirs, link dirs and libraries to both executables,
# and copy the matching OpenCV DLL next to each one after it is built.
foreach(TARGET ${PROJECT_NAME} ${TEST_TARGET})
    target_include_directories(${TARGET} PRIVATE ${OPENCV_INCLUDE_DIR})
    target_link_directories(${TARGET} PRIVATE ${OPENCV_LIB_DIR})

    # Link OpenCV libraries - use different libraries for Debug vs Release
    target_link_libraries(${TARGET}
        $<$<CONFIG:Debug>:${OPENCV_LIB_DIR}/opencv_world451d.lib>
        $<$<CONFIG:Release>:${OPENCV_LIB_DIR}/opencv_world451.lib>
        $<$<CONFIG:RelWithDebInfo>:${OPENCV_LIB_DIR}/opencv_world451.lib>
        $<$<CONFIG:MinSizeRel>:${OPENCV_LIB_DIR}/opencv_world451.lib>
    )

    # Copy OpenCV DLLs to output directory
    if(WIN32)
        add_custom_command(TARGET ${TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<$<CONFIG:Debug>:${OPENCV_LIB_DIR}/opencv_world451d.dll>
            $<$<CONFIG:Release>:${OPENCV_LIB_DIR}/opencv_world451.dll>
            $<$<CONFIG:RelWithDebInfo>:${OPENCV_LIB_DIR}/opencv_world451.dll>
            $<$<CONFIG:MinSizeRel>:${OPENCV_LIB_DIR}/opencv_world451.dll>
            $<TARGET_FILE_DIR:${TARGET}>)
    endif()
endforeach()

# Print found sources for debugging
message(STATUS "Game source files: ${GAME_SOURCES}")
message(STATUS "Test source files: ${TEST_SOURCES}")

# ---- Server (persistence layer; networking comes in a later step) ----
add_subdirectory(server)