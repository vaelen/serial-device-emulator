// Copyright (c) 2025 Andrew C. Young <andrew@vaelen.org>
// SPDX-License-Identifier: MIT

#include "CATParser.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

CATParser::CATParser(YaesuState& state, ISerialPort& serial)
    : _state(state)
    , _serial(serial)
    , _logger(nullptr)
    , _bufLen(0)
{
    memset(_buffer, 0, sizeof(_buffer));
}

void CATParser::reset() {
    _bufLen = 0;
    memset(_buffer, 0, sizeof(_buffer));
}

bool CATParser::update() {
    bool commandProcessed = false;

    while (_serial.available()) {
        char c = _serial.read();

        // Check for terminator
        if (c == CAT_TERMINATOR) {
            _buffer[_bufLen] = '\0';
            if (_bufLen > 0) {
                processCommand();
                commandProcessed = true;
            }
            _bufLen = 0;
            continue;
        }

        // Skip control characters
        if (c < 32) {
            continue;
        }

        // Add to buffer if space available
        if (_bufLen < CAT_BUFFER_SIZE - 1) {
            _buffer[_bufLen++] = toupper(c);
        } else {
            // Buffer overflow, reset
            if (_logger) {
                _logger->log(LogLevel::WARN, "CAT", "Buffer overflow, resetting");
            }
            _bufLen = 0;
        }
    }

    return commandProcessed;
}

void CATParser::processCommand() {
    if (_bufLen < 2) {
        if (_logger) {
            _logger->logf(LogLevel::DEBUG, "CAT", "Command too short: '%s'", _buffer);
        }
        return;
    }

    // Extract 2-character command
    char cmd[3] = {_buffer[0], _buffer[1], '\0'};
    const char* params = (_bufLen > 2) ? &_buffer[2] : "";

    if (_logger) {
        _logger->logf(LogLevel::DEBUG, "CAT", "CMD: %s PARAMS: '%s'", cmd, params);
    }

    bool handled = false;

    // Dispatch to handlers
    if (strcmp(cmd, "FA") == 0) handled = handleFA(params);
    else if (strcmp(cmd, "FB") == 0) handled = handleFB(params);
    else if (strcmp(cmd, "IF") == 0) handled = handleIF(params);
    else if (strcmp(cmd, "ID") == 0) handled = handleID(params);
    else if (strcmp(cmd, "MD") == 0) handled = handleMD(params);
    else if (strcmp(cmd, "PS") == 0) handled = handlePS(params);
    else if (strcmp(cmd, "SM") == 0) handled = handleSM(params);
    else if (strcmp(cmd, "TX") == 0) handled = handleTX(params);
    else if (strcmp(cmd, "RX") == 0) handled = handleRX(params);
    else if (strcmp(cmd, "VS") == 0) handled = handleVS(params);
    else if (strcmp(cmd, "RI") == 0) handled = handleRI(params);
    else if (strcmp(cmd, "XT") == 0) handled = handleXT(params);
    else if (strcmp(cmd, "RD") == 0) handled = handleRD(params);
    else if (strcmp(cmd, "RU") == 0) handled = handleRU(params);
    else if (strcmp(cmd, "AG") == 0) handled = handleAG(params);
    else if (strcmp(cmd, "RG") == 0) handled = handleRG(params);
    else if (strcmp(cmd, "SQ") == 0) handled = handleSQ(params);
    else if (strcmp(cmd, "RM") == 0) handled = handleRM(params);

    if (!handled && _logger) {
        _logger->logf(LogLevel::WARN, "CAT", "Unknown command: %s", cmd);
    }
}

void CATParser::sendResponse(const char* response) {
    _serial.print(response);
    _serial.write(CAT_TERMINATOR);

    if (_logger) {
        _logger->logf(LogLevel::DEBUG, "CAT", "RSP: %s;", response);
    }
}

// Format frequency as 9-digit string (Hz)
void CATParser::formatFrequency(char* buf, uint32_t freq) {
    snprintf(buf, 10, "%09lu", (unsigned long)freq);
}

// Parse 9-digit frequency string
bool CATParser::parseFrequency(const char* str, uint32_t& freq) {
    if (strlen(str) != 9) {
        // Try parsing anyway, some clients use shorter strings
    }

    char* endptr;
    unsigned long val = strtoul(str, &endptr, 10);

    if (*endptr != '\0' && *endptr != CAT_TERMINATOR) {
        return false;
    }

    if (val < FREQ_MIN || val > FREQ_MAX) {
        return false;
    }

    freq = (uint32_t)val;
    return true;
}

// FA - VFO-A Frequency
bool CATParser::handleFA(const char* params) {
    if (strlen(params) == 0) {
        // Read
        char buf[16];
        snprintf(buf, sizeof(buf), "FA%09lu", (unsigned long)_state.freqVfoA);
        sendResponse(buf);
    } else {
        // Set
        uint32_t freq;
        if (parseFrequency(params, freq)) {
            _state.freqVfoA = freq;
        }
    }
    return true;
}

// FB - VFO-B Frequency
bool CATParser::handleFB(const char* params) {
    if (strlen(params) == 0) {
        // Read
        char buf[16];
        snprintf(buf, sizeof(buf), "FB%09lu", (unsigned long)_state.freqVfoB);
        sendResponse(buf);
    } else {
        // Set
        uint32_t freq;
        if (parseFrequency(params, freq)) {
            _state.freqVfoB = freq;
        }
    }
    return true;
}

// Format IF response string
// Format: IFaaaaaaaaabbbbcccccddeeeffghijklm
// aaaa = freq (9 digits), bbbb = ?, ccccc = RIT offset, dd = RIT on, ee = XIT on, etc.
void CATParser::formatIF(char* buf) {
    uint32_t freq = _state.getCurrentFreq();
    int16_t ritOffset = _state.ritOn ? _state.ritOffset : 0;

    // Build IF response per FT-991A format
    // Position: 01234567890123456789012345678
    // Format:   IFnnnnnnnnn+oooo0mm0000000000
    snprintf(buf, 32, "IF%09lu%+05d0%02d0000000000",
             (unsigned long)freq,
             ritOffset,
             (uint8_t)_state.getCurrentMode());
}

// IF - Information
bool CATParser::handleIF(const char* params) {
    (void)params;  // IF is read-only

    char buf[32];
    formatIF(buf);
    sendResponse(buf);
    return true;
}

// ID - Radio ID
bool CATParser::handleID(const char* params) {
    (void)params;  // ID is read-only

    char buf[8];
    snprintf(buf, sizeof(buf), "ID%s", YAESU_RADIO_ID);
    sendResponse(buf);
    return true;
}

// MD - Mode
bool CATParser::handleMD(const char* params) {
    if (strlen(params) == 0) {
        // Read current VFO mode
        char buf[8];
        snprintf(buf, sizeof(buf), "MD0%d", (int)_state.getCurrentMode());
        sendResponse(buf);
    } else if (strlen(params) >= 2) {
        // Set: MD0n where 0=main, n=mode
        int mode = params[1] - '0';
        if (mode >= 1 && mode <= 14) {
            _state.setCurrentMode((YaesuMode)mode);
        }
    }
    return true;
}

// PS - Power Status
bool CATParser::handlePS(const char* params) {
    if (strlen(params) == 0) {
        // Read
        char buf[4];
        snprintf(buf, sizeof(buf), "PS%d", _state.powerOn ? 1 : 0);
        sendResponse(buf);
    } else {
        // Set
        _state.powerOn = (params[0] == '1');
    }
    return true;
}

// SM - S-Meter
bool CATParser::handleSM(const char* params) {
    // SM0; reads main receiver S-meter
    // Response: SM0nnn; where nnn = 000-255
    (void)params;

    char buf[8];
    snprintf(buf, sizeof(buf), "SM0%03d", _state.smeter);
    sendResponse(buf);
    return true;
}

// TX - Transmit
bool CATParser::handleTX(const char* params) {
    if (strlen(params) == 0) {
        // Read
        char buf[4];
        snprintf(buf, sizeof(buf), "TX%d", _state.ptt ? 1 : 0);
        sendResponse(buf);
    } else {
        // Set: TX0=off, TX1=on, TX2=tune
        _state.ptt = (params[0] != '0');
    }
    return true;
}

// RX - Receive
bool CATParser::handleRX(const char* params) {
    (void)params;
    _state.ptt = false;
    return true;
}

// VS - VFO Select
bool CATParser::handleVS(const char* params) {
    if (strlen(params) == 0) {
        // Read
        char buf[8];
        snprintf(buf, sizeof(buf), "VS%d", (int)_state.currentVfo);
        sendResponse(buf);
    } else {
        // Set: 0=VFO-A, 1=VFO-B
        _state.currentVfo = (params[0] == '0') ? YaesuVFO::VFO_A : YaesuVFO::VFO_B;
    }
    return true;
}

// RI - RIT On/Off
bool CATParser::handleRI(const char* params) {
    if (strlen(params) == 0) {
        // Read
        char buf[4];
        snprintf(buf, sizeof(buf), "RI%d", _state.ritOn ? 1 : 0);
        sendResponse(buf);
    } else {
        // Set
        _state.ritOn = (params[0] == '1');
    }
    return true;
}

// XT - XIT On/Off
bool CATParser::handleXT(const char* params) {
    if (strlen(params) == 0) {
        // Read
        char buf[4];
        snprintf(buf, sizeof(buf), "XT%d", _state.xitOn ? 1 : 0);
        sendResponse(buf);
    } else {
        // Set
        _state.xitOn = (params[0] == '1');
    }
    return true;
}

// RD - RIT Down
bool CATParser::handleRD(const char* params) {
    if (strlen(params) >= 4) {
        // Set absolute offset
        int16_t offset = atoi(params);
        _state.ritOffset = constrain(offset, -9999, 9999);
    } else {
        // Decrement by default step
        _state.ritOffset = constrain(_state.ritOffset - 10, -9999, 9999);
    }
    return true;
}

// RU - RIT Up
bool CATParser::handleRU(const char* params) {
    if (strlen(params) >= 4) {
        // Set absolute offset
        int16_t offset = atoi(params);
        _state.ritOffset = constrain(offset, -9999, 9999);
    } else {
        // Increment by default step
        _state.ritOffset = constrain(_state.ritOffset + 10, -9999, 9999);
    }
    return true;
}

// AG - AF Gain
bool CATParser::handleAG(const char* params) {
    if (strlen(params) == 0 || strlen(params) == 1) {
        // Read: AG0; where 0=main
        char buf[8];
        snprintf(buf, sizeof(buf), "AG0%03d", _state.afGain);
        sendResponse(buf);
    } else if (strlen(params) >= 4) {
        // Set: AG0nnn
        int gain = atoi(&params[1]);
        _state.afGain = constrain(gain, 0, 255);
    }
    return true;
}

// RG - RF Gain
bool CATParser::handleRG(const char* params) {
    if (strlen(params) == 0 || strlen(params) == 1) {
        // Read
        char buf[8];
        snprintf(buf, sizeof(buf), "RG0%03d", _state.rfGain);
        sendResponse(buf);
    } else if (strlen(params) >= 4) {
        // Set: RG0nnn
        int gain = atoi(&params[1]);
        _state.rfGain = constrain(gain, 0, 255);
    }
    return true;
}

// SQ - Squelch
bool CATParser::handleSQ(const char* params) {
    if (strlen(params) == 0 || strlen(params) == 1) {
        // Read
        char buf[8];
        snprintf(buf, sizeof(buf), "SQ0%03d", _state.squelch);
        sendResponse(buf);
    } else if (strlen(params) >= 4) {
        // Set: SQ0nnn
        int sq = atoi(&params[1]);
        _state.squelch = constrain(sq, 0, 100);
    }
    return true;
}

// RM - Read Meter
bool CATParser::handleRM(const char* params) {
    // RM1; = S-meter, RM2; = Power, RM3; = SWR, etc.
    int meter = 1;
    if (strlen(params) >= 1) {
        meter = params[0] - '0';
    }

    uint8_t value = 0;
    switch (meter) {
        case 1: value = _state.smeter; break;
        case 2: value = _state.powerMeter; break;
        case 3: value = _state.swrMeter; break;
        case 4: value = _state.alcMeter; break;
        case 5: value = _state.compMeter; break;
        default: value = 0; break;
    }

    char buf[16];
    snprintf(buf, sizeof(buf), "RM%d%03d", meter, value);
    sendResponse(buf);
    return true;
}
