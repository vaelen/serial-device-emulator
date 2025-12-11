// Copyright (c) 2025 Andrew C. Young <andrew@vaelen.org>
// SPDX-License-Identifier: MIT

#include "GS232Parser.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

GS232Parser::GS232Parser(G5500State& state, ISerialPort& serial)
    : _state(state)
    , _serial(serial)
    , _logger(nullptr)
    , _echo(false)
    , _bufLen(0)
{
    memset(_buffer, 0, sizeof(_buffer));
}

void GS232Parser::reset() {
    _bufLen = 0;
    memset(_buffer, 0, sizeof(_buffer));
}

bool GS232Parser::update() {
    bool processed = false;

    while (_serial.available()) {
        char c = _serial.read();

        // Echo character if enabled
        if (_echo && _logger) {
            char echoStr[2] = {c, '\0'};
            if (c == GS232_CR) {
                _logger->logf(LogLevel::DEBUG, "G5500", "<CR>");
            } else if (c == GS232_LF) {
                _logger->logf(LogLevel::DEBUG, "G5500", "<LF>");
            } else if (c >= 32 && c < 127) {
                _logger->logf(LogLevel::DEBUG, "G5500", "RX: %s", echoStr);
            }
        }

        // GS-232 uses CR as command terminator
        if (c == GS232_CR || c == GS232_LF) {
            if (_bufLen > 0) {
                _buffer[_bufLen] = '\0';
                processCommand();
                _bufLen = 0;
                processed = true;
            }
            continue;
        }

        // Add character to buffer (convert to uppercase)
        if (_bufLen < CAT_BUFFER_SIZE - 1 && c >= 32 && c < 127) {
            _buffer[_bufLen++] = toupper(c);
        }
    }

    return processed;
}

void GS232Parser::processCommand() {
    if (_bufLen == 0) return;

    if (_logger) {
        _logger->logf(LogLevel::DEBUG, "G5500", "CMD: %s", _buffer);
    }

    // Single character commands
    char cmd = _buffer[0];

    switch (cmd) {
        case 'R':
            handleR();
            break;
        case 'L':
            handleL();
            break;
        case 'A':
            handleA();
            break;
        case 'U':
            handleU();
            break;
        case 'D':
            handleD();
            break;
        case 'E':
            handleE();
            break;
        case 'S':
            handleS();
            break;
        case 'C':
            handleC();
            break;
        case 'B':
            handleB();
            break;
        case 'M':
            handleM(_buffer + 1);
            break;
        case 'W':
            handleW(_buffer + 1);
            break;
        default:
            // Unknown command - GS-232 typically ignores unknown commands
            if (_logger) {
                _logger->logf(LogLevel::WARN, "G5500", "Unknown command: %s", _buffer);
            }
            break;
    }
}

void GS232Parser::sendResponse(const char* response) {
    _serial.print(response);
    _serial.print("\r\n");

    if (_echo && _logger) {
        _logger->logf(LogLevel::DEBUG, "G5500", "TX: %s", response);
    }
}

// R - Rotate CW (clockwise, increasing azimuth)
void GS232Parser::handleR() {
    _state.rotateCW();
    if (_logger) {
        _logger->logf(LogLevel::DEBUG, "G5500", "Rotating CW");
    }
}

// L - Rotate CCW (counter-clockwise, decreasing azimuth)
void GS232Parser::handleL() {
    _state.rotateCCW();
    if (_logger) {
        _logger->logf(LogLevel::DEBUG, "G5500", "Rotating CCW");
    }
}

// A - Stop azimuth rotation
void GS232Parser::handleA() {
    _state.stopAzimuth();
    if (_logger) {
        _logger->logf(LogLevel::DEBUG, "G5500", "Azimuth stopped");
    }
}

// U - Rotate up (increasing elevation)
void GS232Parser::handleU() {
    _state.rotateUp();
    if (_logger) {
        _logger->logf(LogLevel::DEBUG, "G5500", "Rotating up");
    }
}

// D - Rotate down (decreasing elevation)
void GS232Parser::handleD() {
    _state.rotateDown();
    if (_logger) {
        _logger->logf(LogLevel::DEBUG, "G5500", "Rotating down");
    }
}

// E - Stop elevation rotation
void GS232Parser::handleE() {
    _state.stopElevation();
    if (_logger) {
        _logger->logf(LogLevel::DEBUG, "G5500", "Elevation stopped");
    }
}

// S - Full stop (both azimuth and elevation)
void GS232Parser::handleS() {
    _state.stopAll();
    if (_logger) {
        _logger->logf(LogLevel::DEBUG, "G5500", "All rotation stopped");
    }
}

// C - Read azimuth, C2 - Read azimuth and elevation
void GS232Parser::handleC() {
    char response[32];

    // Check for C2 command (read both az and el)
    if (_bufLen >= 2 && _buffer[1] == '2') {
        // Response format: +0xxx +0xxx (azimuth elevation)
        snprintf(response, sizeof(response), "+0%03d +0%03d",
                 _state.getAzimuthInt(), _state.getElevationInt());
    } else {
        // Response format: +0xxx (azimuth only)
        snprintf(response, sizeof(response), "+0%03d", _state.getAzimuthInt());
    }

    sendResponse(response);
}

// B - Read elevation
void GS232Parser::handleB() {
    char response[16];
    snprintf(response, sizeof(response), "+0%03d", _state.getElevationInt());
    sendResponse(response);
}

// M - Move to azimuth (Mxxx where xxx is 000-450)
void GS232Parser::handleM(const char* params) {
    int angle;
    if (!parseAngle(params, angle)) {
        if (_logger) {
            _logger->logf(LogLevel::WARN, "G5500", "Invalid azimuth in M command: %s", params);
        }
        return;
    }

    // Validate azimuth range
    if (angle < (int)AZ_MIN || angle > (int)AZ_MAX) {
        if (_logger) {
            _logger->logf(LogLevel::WARN, "G5500", "Azimuth out of range: %d", angle);
        }
        return;
    }

    _state.gotoAzimuth((float)angle);
    if (_logger) {
        _logger->logf(LogLevel::DEBUG, "G5500", "Moving to azimuth %d", angle);
    }
}

// W - Move to azimuth and elevation (Wxxx yyy where xxx=azimuth, yyy=elevation)
void GS232Parser::handleW(const char* params) {
    // Find space separator between azimuth and elevation
    const char* space = strchr(params, ' ');
    if (space == nullptr) {
        if (_logger) {
            _logger->logf(LogLevel::WARN, "G5500", "Invalid W command format: %s", params);
        }
        return;
    }

    // Parse azimuth (before space)
    char azStr[8];
    size_t azLen = space - params;
    if (azLen >= sizeof(azStr)) azLen = sizeof(azStr) - 1;
    strncpy(azStr, params, azLen);
    azStr[azLen] = '\0';

    int azAngle;
    if (!parseAngle(azStr, azAngle)) {
        if (_logger) {
            _logger->logf(LogLevel::WARN, "G5500", "Invalid azimuth in W command: %s", azStr);
        }
        return;
    }

    // Parse elevation (after space)
    int elAngle;
    if (!parseAngle(space + 1, elAngle)) {
        if (_logger) {
            _logger->logf(LogLevel::WARN, "G5500", "Invalid elevation in W command: %s", space + 1);
        }
        return;
    }

    // Validate ranges
    if (azAngle < (int)AZ_MIN || azAngle > (int)AZ_MAX) {
        if (_logger) {
            _logger->logf(LogLevel::WARN, "G5500", "Azimuth out of range: %d", azAngle);
        }
        return;
    }
    if (elAngle < (int)EL_MIN || elAngle > (int)EL_MAX) {
        if (_logger) {
            _logger->logf(LogLevel::WARN, "G5500", "Elevation out of range: %d", elAngle);
        }
        return;
    }

    _state.gotoAzimuth((float)azAngle);
    _state.gotoElevation((float)elAngle);

    if (_logger) {
        _logger->logf(LogLevel::DEBUG, "G5500", "Moving to az=%d el=%d", azAngle, elAngle);
    }
}

bool GS232Parser::parseAngle(const char* str, int& angle) {
    if (str == nullptr || *str == '\0') {
        return false;
    }

    // Skip leading whitespace
    while (*str == ' ') str++;

    // Parse the number
    char* endptr;
    long val = strtol(str, &endptr, 10);

    // Check if any digits were parsed
    if (endptr == str) {
        return false;
    }

    angle = (int)val;
    return true;
}
