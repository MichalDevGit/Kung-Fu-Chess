# Containerizes today's server-side processes exactly as they run outside
# Docker -- see MIGRATION_PLAN.md Phase 0 (this file) and Phase 2 (the
# apigateway addition below).
#
# Only server-side executables are built here: client/ links the vendored
# Windows/MSVC OpenCV binaries under lib/ (see ARCHITECTURE.md "Build
# system"), which can't configure or link on Linux at all, so this image
# passes -DBUILD_CLIENT=OFF (see root CMakeLists.txt) to skip
# add_subdirectory(client) entirely instead of failing partway through a
# build nothing here needs.
#
# libpq-dev (build stage) / libpq5 (runtime stages): MIGRATION_PLAN.md Phase 1's
# PostgresUserRepository is only compiled in where find_package(PostgreSQL)
# succeeds (see server/CMakeLists.txt) -- this is the one place that's always
# true, so this image is what makes the Postgres backend real. As of Phase 2,
# Postgres is no longer merely "reachable but unused": once KungFuChessServer
# and KungFuChessApiGateway are separate containers, they must be pointed at
# the SAME user store or they'd silently diverge (see docker-compose.yml,
# which sets the same KUNGFUCHESS_POSTGRES_URL on both services).

# ---- Build stage (shared by both runtime images below) ----
FROM ubuntu:22.04 AS build

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        git \
        ca-certificates \
        libpq-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src

# Only what a server-only (-DBUILD_CLIENT=OFF) build actually touches --
# client/ and lib/ (vendored OpenCV) are deliberately not copied into the
# build context at all, since BUILD_CLIENT=OFF means add_subdirectory(client)
# never runs and neither is ever read.
COPY CMakeLists.txt ./
COPY common ./common
COPY protocol ./protocol
COPY server ./server
COPY apigateway ./apigateway

# FetchContent (ixwebsocket/nlohmann_json/SQLiteCpp/bcrypt/hiredis/
# redis-plus-plus) needs network access at configure time, same as a local
# dev build -- nothing here is vendored differently for Docker.
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_CLIENT=OFF \
    && cmake --build build --target KungFuChessServer KungFuChessApiGateway --parallel

# ---- Runtime stage: KungFuChessServer (the WebSocket game process) ----
FROM ubuntu:22.04 AS runtime-server

RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates \
        curl \
        libpq5 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=build /src/build/server/KungFuChessServer ./KungFuChessServer

# Accept connections from outside the container (the default "127.0.0.1"
# bind would only accept connections from inside the container itself,
# making a published port unreachable -- see NetworkConfig.h /
# server/src/main.cpp).
ENV KUNGFUCHESS_HOST=0.0.0.0

# The SQLite file (server/src/main.cpp opens "kungfuchess.db" relative to the
# process's working directory) lives on a mounted volume so it survives a
# container recreate, same as the plan's "SQLite file on a mounted volume".
# Irrelevant once KUNGFUCHESS_POSTGRES_URL is set (see docker-compose.yml).
RUN mkdir -p /app/data
VOLUME ["/app/data"]
WORKDIR /app/data

EXPOSE 9002 9003

HEALTHCHECK --interval=10s --timeout=3s --start-period=5s --retries=3 \
    CMD curl -f http://127.0.0.1:9003/health || exit 1

ENTRYPOINT ["/app/KungFuChessServer"]

# ---- Runtime stage: KungFuChessApiGateway (MIGRATION_PLAN.md Phase 2) ----
FROM ubuntu:22.04 AS runtime-apigateway

RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates \
        curl \
        libpq5 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=build /src/build/apigateway/KungFuChessApiGateway ./KungFuChessApiGateway

ENV KUNGFUCHESS_HOST=0.0.0.0

# Same SQLite-file-on-a-volume story as runtime-server above -- irrelevant
# once KUNGFUCHESS_POSTGRES_URL is set. NOT the same volume as
# runtime-server's, since each writes its own local "kungfuchess.db" only if
# Postgres isn't configured; docker-compose.yml sets KUNGFUCHESS_POSTGRES_URL
# on both services specifically so this never matters in practice.
RUN mkdir -p /app/data
VOLUME ["/app/data"]
WORKDIR /app/data

EXPOSE 9004

ENTRYPOINT ["/app/KungFuChessApiGateway"]
