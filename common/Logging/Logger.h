#ifndef COMMON_LOGGING_LOGGER_H
#define COMMON_LOGGING_LOGGER_H

#include <string>

#include "common/Logging/LogLevel.h"

// Minimal leveled, timestamped logger -- Phase 0 "groundwork" observability
// (see MIGRATION_PLAN.md), replacing ad hoc std::cout lines in server/src/main.cpp
// and server/src/network/WebSocketServer.cpp with something a container log
// driver/log aggregator can filter on by level. Deliberately not a full
// logging framework (no sinks/config/async queue) -- just enough structure to
// be useful today, and swappable later without touching call sites.
namespace common
{
class Logger
{
public:
    static void debug(const std::string& message);
    static void info(const std::string& message);
    static void warn(const std::string& message);
    static void error(const std::string& message);

    // Pure formatting (timestamp + level tag), exposed separately from the
    // actual stdout/stderr write so it can be unit-tested without capturing
    // process output.
    static std::string format(LogLevel level, const std::string& message);

private:
    static void log(LogLevel level, const std::string& message);
};
}

#endif
