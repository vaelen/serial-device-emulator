// Copyright (c) 2025 Andrew C. Young <andrew@vaelen.org>
// SPDX-License-Identifier: MIT

#pragma once

#include <Arduino.h>
#include "platform_config.h"
#include "G5500State.h"
#include "ISerialPort.h"
#include "ILogger.h"

// GS-232 command terminators
#define GS232_CR '\r'
#define GS232_LF '\n'

// Parser for Yaesu GS-232A/B rotator commands
class GS232Parser {
public:
    GS232Parser(G5500State& state, ISerialPort& serial);

    void setLogger(ILogger* logger) { _logger = logger; }
    void setEcho(bool echo) { _echo = echo; }

    // Process any available input (non-blocking)
    // Returns true if a command was processed
    bool update();

    // Reset input buffer
    void reset();

private:
    G5500State& _state;
    ISerialPort& _serial;
    ILogger* _logger;
    bool _echo;

    char _buffer[CAT_BUFFER_SIZE];
    size_t _bufLen;

    // Process a complete command line
    void processCommand();

    // Send response string (with CR LF terminator)
    void sendResponse(const char* response);

    // Command handlers
    void handleR();                      // Rotate CW (right)
    void handleL();                      // Rotate CCW (left)
    void handleA();                      // Stop azimuth
    void handleU();                      // Rotate up
    void handleD();                      // Rotate down
    void handleE();                      // Stop elevation
    void handleS();                      // Full stop
    void handleC();                      // Read azimuth (C) or azimuth+elevation (C2)
    void handleB();                      // Read elevation
    void handleM(const char* params);    // Move to azimuth
    void handleW(const char* params);    // Move to azimuth and elevation

    // Parse 3-digit angle value from string
    bool parseAngle(const char* str, int& angle);
};
