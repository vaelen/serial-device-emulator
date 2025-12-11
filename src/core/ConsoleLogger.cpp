// Copyright (c) 2025 Andrew C. Young <andrew@vaelen.org>
// SPDX-License-Identifier: MIT

#include "ConsoleLogger.h"
#include <stdio.h>

ConsoleLogger::ConsoleLogger(Stream& output)
    : _output(output)
    , _minLevel(LogLevel::INFO)
{
}

void ConsoleLogger::printPrefix(LogLevel level, const char* tag) {
    _output.print('[');
    _output.print(logLevelToString(level));
    _output.print("] [");
    _output.print(tag);
    _output.print("] ");
}

void ConsoleLogger::log(LogLevel level, const char* tag, const char* message) {
    if (level < _minLevel) {
        return;
    }

    printPrefix(level, tag);
    _output.println(message);
}

void ConsoleLogger::logf(LogLevel level, const char* tag, const char* fmt, ...) {
    if (level < _minLevel) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    vsnprintf(_buffer, sizeof(_buffer), fmt, args);
    va_end(args);

    printPrefix(level, tag);
    _output.println(_buffer);
}
