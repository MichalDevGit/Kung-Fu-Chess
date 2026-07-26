#ifndef COMMON_CONFIG_NETWORK_CONFIG_H
#define COMMON_CONFIG_NETWORK_CONFIG_H

// Single source of truth for where the server listens and where the client
// connects. Previously the host/port were separately hardcoded in
// server/src/main.cpp and client/src/main.cpp.
namespace NetworkConfig
{
    constexpr const char* DEFAULT_HOST = "127.0.0.1";
    constexpr int DEFAULT_PORT = 9002;
}

#endif
