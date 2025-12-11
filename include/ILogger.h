// Copyright (c) 2025 Andrew C. Young <andrew@vaelen.org>
// SPDX-License-Identifier: MIT

#pragma once

#include <Arduino.h>

enum class LogLevel : uint8_t {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3
};

// Convert LogLevel to string
inline const char* logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DBG";
        case LogLevel::INFO:  return "INF";
        case LogLevel::WARN:  return "WRN";
        case LogLevel::ERROR: return "ERR";
        default: return "???";
    }
}

// Parse string to LogLevel, returns true on success
inline bool parseLogLevel(const char* str, LogLevel& level) {
    if (strcasecmp(str, "debug") == 0) { level = LogLevel::DEBUG; return true; }
    if (strcasecmp(str, "info") == 0)  { level = LogLevel::INFO;  return true; }
    if (strcasecmp(str, "warn") == 0)  { level = LogLevel::WARN;  return true; }
    if (strcasecmp(str, "error") == 0) { level = LogLevel::ERROR; return true; }
    return false;
}

// Abstract logger interface for devices to communicate with console
class ILogger {
public:
    virtual ~ILogger() = default;

    // Log a formatted message (printf-style)
    virtual void logf(LogLevel level, const char* tag, const char* fmt, ...) = 0;

    // Get current minimum log level
    virtual LogLevel getLevel() const = 0;

    // Set minimum log level (messages below this level are discarded)
    virtual void setLevel(LogLevel level) = 0;
};
