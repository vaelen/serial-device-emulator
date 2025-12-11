// Copyright (c) 2025 Andrew C. Young <andrew@vaelen.org>
// SPDX-License-Identifier: MIT

#pragma once

#include <Arduino.h>
#include "platform_config.h"
#include "YaesuState.h"
#include "ISerialPort.h"
#include "ILogger.h"

// CAT command terminator
#define CAT_TERMINATOR ';'

// Parser for Yaesu CAT commands
class CATParser {
public:
    CATParser(YaesuState& state, ISerialPort& serial);

    // Set logger for debug output
    void setLogger(ILogger* logger) { _logger = logger; }

    // Process any available input (non-blocking)
    // Returns true if a command was processed
    bool update();

    // Reset input buffer
    void reset();

private:
    YaesuState& _state;
    ISerialPort& _serial;
    ILogger* _logger;

    char _buffer[CAT_BUFFER_SIZE];
    size_t _bufLen;

    // Process a complete command
    void processCommand();

    // Send response string
    void sendResponse(const char* response);

    // Command handlers (return true if command was recognized)
    bool handleFA(const char* params);  // VFO-A frequency
    bool handleFB(const char* params);  // VFO-B frequency
    bool handleIF(const char* params);  // Information
    bool handleID(const char* params);  // Radio ID
    bool handleMD(const char* params);  // Mode
    bool handlePS(const char* params);  // Power status
    bool handleSM(const char* params);  // S-meter
    bool handleTX(const char* params);  // PTT
    bool handleRX(const char* params);  // Receive mode
    bool handleVS(const char* params);  // VFO select
    bool handleRI(const char* params);  // RIT on/off
    bool handleXT(const char* params);  // XIT on/off
    bool handleRD(const char* params);  // RIT down
    bool handleRU(const char* params);  // RIT up
    bool handleAG(const char* params);  // AF gain
    bool handleRG(const char* params);  // RF gain
    bool handleSQ(const char* params);  // Squelch
    bool handleRM(const char* params);  // Read meter

    // Utility functions
    void formatFrequency(char* buf, uint32_t freq);
    bool parseFrequency(const char* str, uint32_t& freq);
    void formatIF(char* buf);
};
