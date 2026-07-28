# API Gateway (MIGRATION_PLAN.md Phase 2): register/login split out of the
# WebSocket process into their own small, independently deployable HTTP
# service. Reuses AuthService/RepositoryFactory verbatim from server/
# (KungFuChessAuth/KungFuChessPersistence, already built there) instead of
# duplicating auth business logic -- the only new code here is the REST
# request/response translation (ApiGatewayRequestHandler) and the HTTP
# transport wrapper (ApiGatewayServer). ixwebsocket/nlohmann_json are
# fetched/defined once in the root CMakeLists.txt; KungFuChessAuth/
# KungFuChessPersistence/KungFuChessCommon/KungFuChessProtocol come from
# common/ and server/, both already add_subdirectory'd before this one.

file(GLOB APIGATEWAY_LIB_SOURCES "src/ApiGatewayRequestHandler.cpp" "src/network/ApiGatewayServer.cpp")
file(GLOB APIGATEWAY_LIB_HEADERS "src/ApiGatewayRequestHandler.h" "src/network/ApiGatewayServer.h")

add_library(KungFuChessApiGatewayLib STATIC ${APIGATEWAY_LIB_SOURCES} ${APIGATEWAY_LIB_HEADERS})
target_include_directories(KungFuChessApiGatewayLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(KungFuChessApiGatewayLib PUBLIC KungFuChessAuth KungFuChessCommon KungFuChessProtocol ixwebsocket::ixwebsocket)

# ---- Executable (serves REST auth over http://127.0.0.1:9004) ----
add_executable(KungFuChessApiGateway src/main.cpp)
target_link_libraries(KungFuChessApiGateway PRIVATE KungFuChessApiGatewayLib)

# ---- Tests (request handler only -- no real HTTP server/database, same
# InMemoryUserRepository-backed pattern server/tests/auth_service_test.cpp
# already uses) ----
file(GLOB APIGATEWAY_TEST_SOURCES "tests/*.cpp")

add_executable(KungFuChessApiGateway_tests ${APIGATEWAY_TEST_SOURCES})
target_include_directories(KungFuChessApiGateway_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(KungFuChessApiGateway_tests PRIVATE KungFuChessApiGatewayLib KungFuChessPersistence)
