// Copyright (c) 2025 Andrew C. Young <andrew@vaelen.org>
// SPDX-License-Identifier: MIT

#pragma once

#include "ILogger.h"
#include "platform_config.h"
#include <stdarg.h>

// Logger implementation that outputs to the console serial port
class ConsoleLogger : public ILogger {
public:
    explicit ConsoleLogger(Stream& output);
    ~ConsoleLogger() override = default;

    void logf(LogLevel level, const char* tag, const char* fmt, ...) override;

    LogLevel getLevel() const override { return _minLevel; }
    void setLevel(LogLevel level) override { _minLevel = level; }

private:
    Stream& _output;
    LogLevel _minLevel;
    char _buffer[LOG_BUFFER_SIZE];

    void printPrefix(LogLevel level, const char* tag);
};
