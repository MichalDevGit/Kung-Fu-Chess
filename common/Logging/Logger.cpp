#include "common/Logging/Logger.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>

namespace common
{
namespace
{
// Guards stdout/stderr so log lines from different threads (the server's tick
// thread, ix's per-connection callback threads) never interleave mid-line --
// the same category of problem CliShell's own output mutex solves on the
// client side, just for log lines instead of interactive console output.
std::mutex logMutex;

std::string currentTimestampUtc()
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t nowTimeT = std::chrono::system_clock::to_time_t(now);
    const auto millis =
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;

    std::tm utcTime{};
#if defined(_WIN32)
    gmtime_s(&utcTime, &nowTimeT);
#else
    gmtime_r(&nowTimeT, &utcTime);
#endif

    std::ostringstream stream;
    stream << std::put_time(&utcTime, "%Y-%m-%dT%H:%M:%S") << '.' << std::setfill('0') << std::setw(3)
           << millis << 'Z';
    return stream.str();
}
}

std::string Logger::format(LogLevel level, const std::string& message)
{
    std::ostringstream stream;
    stream << '[' << currentTimestampUtc() << "] [" << toString(level) << "] " << message;
    return stream.str();
}

void Logger::log(LogLevel level, const std::string& message)
{
    const std::string line = format(level, message);

    std::lock_guard<std::mutex> lock(logMutex);
    // WARN/ERROR to stderr, DEBUG/INFO to stdout -- lets `docker logs`/a log
    // collector split severity by stream without parsing the level tag.
    if (level == LogLevel::Warn || level == LogLevel::Error)
        std::cerr << line << std::endl;
    else
        std::cout << line << std::endl;
}

void Logger::debug(const std::string& message)
{
    log(LogLevel::Debug, message);
}

void Logger::info(const std::string& message)
{
    log(LogLevel::Info, message);
}

void Logger::warn(const std::string& message)
{
    log(LogLevel::Warn, message);
}

void Logger::error(const std::string& message)
{
    log(LogLevel::Error, message);
}
}
