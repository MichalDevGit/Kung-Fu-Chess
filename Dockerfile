# Containerizes today's monolith (KungFuChessServer) exactly as it runs
# outside Docker -- no architecture change, see MIGRATION_PLAN.md Phase 0.
#
# Only the server is built here: client/ links the vendored Windows/MSVC
# OpenCV binaries under lib/ (see ARCHITECTURE.md "Build system"), which
# can't configure or link on Linux at all, so this image passes
# -DBUILD_CLIENT=OFF (see root CMakeLists.txt) to skip add_subdirectory(client)
# entirely instead of failing partway through a build nothing here needs.

# ---- Build stage ----
FROM ubuntu:22.04 AS build

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        git \
        ca-certificates \
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

# FetchContent (ixwebsocket/nlohmann_json/SQLiteCpp/bcrypt) needs network
# access at configure time, same as a local dev build -- nothing here is
# vendored differently for Docker.
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_CLIENT=OFF \
    && cmake --build build --target KungFuChessServer --parallel

# ---- Runtime stage ----
FROM ubuntu:22.04 AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates \
        curl \
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
RUN mkdir -p /app/data
VOLUME ["/app/data"]
WORKDIR /app/data

EXPOSE 9002 9003

HEALTHCHECK --interval=10s --timeout=3s --start-period=5s --retries=3 \
    CMD curl -f http://127.0.0.1:9003/health || exit 1

ENTRYPOINT ["/app/KungFuChessServer"]
