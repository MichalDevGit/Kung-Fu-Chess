#ifndef COMMON_LOGGING_LOG_LEVEL_H
#define COMMON_LOGGING_LOG_LEVEL_H

namespace common
{
enum class LogLevel
{
    Debug,
    Info,
    Warn,
    Error
};

inline const char* toString(LogLevel level)
{
    switch (level)
    {
        case LogLevel::Debug:
            return "DEBUG";
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warn:
            return "WARN";
        case LogLevel::Error:
            return "ERROR";
    }
    return "UNKNOWN";
}
}

#endif
