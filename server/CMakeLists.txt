# KungFuChess server: persistence layer (step 1).
# No networking/Asio/room code yet -- see ARCHITECTURE.md's "Server / persistence"
# section for the roadmap this is the first slice of.

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

# ---- Persistence tests ----
file(GLOB PERSISTENCE_TEST_SOURCES "tests/*.cpp")

add_executable(KungFuChessPersistence_tests ${PERSISTENCE_TEST_SOURCES})
target_include_directories(KungFuChessPersistence_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(KungFuChessPersistence_tests PRIVATE KungFuChessPersistence)

# ---- Server executable (creates/inspects the SQLite DB file on disk) ----
add_executable(KungFuChessServer src/main.cpp)
target_link_libraries(KungFuChessServer PRIVATE KungFuChessPersistence)
