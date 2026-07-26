# Shared, transport-agnostic types: view/DTO structs, enums, the generic
# EventBus, and centralized configuration constants (board/timing/network).
# Promoted out of src/common so it's no longer tied to the (retiring) local
# KungFuChess executable's build -- server/ and client/ each link this
# instead of duplicating these types or their constants.
#
# A real STATIC library, not header-only INTERFACE: several DTOs (BoardView,
# PieceView, PositionView, MotionView, JumpView, RestView, GameView) and
# PixelPosition have .cpp implementations, not just declarations.

file(GLOB_RECURSE COMMON_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE COMMON_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_library(KungFuChessCommon STATIC ${COMMON_SOURCES} ${COMMON_HEADERS})
target_include_directories(KungFuChessCommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
