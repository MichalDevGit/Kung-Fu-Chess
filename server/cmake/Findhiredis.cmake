# Hand-written Module-mode find_package shim for hiredis, used only so that
# redis-plus-plus's own internal `find_package(hiredis QUIET)` (see its
# cmake/FindHiredis.cmake) resolves to the hiredis this project already
# FetchContent-fetched/built itself, instead of searching the system.
#
# Why this exists instead of FetchContent's OVERRIDE_FIND_PACKAGE (which does
# exactly this, more directly): that feature needs CMake >= 3.24, but the
# Docker/Linux build's apt-installed cmake (Ubuntu 22.04) is 3.22 -- and
# CMake's own default find_package search tries Module mode (any
# Find<Name>.cmake on CMAKE_MODULE_PATH) *before* Config mode, for any CMake
# version, so this works everywhere without a version floor.
#
# hiredis_INCLUDE_DIRS points at server/CMakeLists.txt's staged copy of
# hiredis's public headers (${HIREDIS_STAGED_INCLUDE_DIR}, set as a normal
# variable before this module is ever invoked) -- see that file's own
# comment for why the staging step itself is needed (hiredis's raw source
# layout doesn't match the installed-style layout redis-plus-plus's CMake
# expects). The real hiredis::hiredis target already exists (aliased in
# hiredis's own CMakeLists.txt) from this project's own
# FetchContent_MakeAvailable(hiredis) call, so nothing further is needed for
# linking.
set(hiredis_FOUND TRUE)
set(hiredis_INCLUDE_DIRS "${HIREDIS_STAGED_INCLUDE_DIR}")
