// Copyright (c) 2025 Andrew C. Young <andrew@vaelen.org>
// SPDX-License-Identifier: MIT

#pragma once

#include <Arduino.h>
#include "NMEAGPSState.h"
#include "ISerialPort.h"
#include "ILogger.h"

// NMEA sentence buffer size
#define NMEA_SENTENCE_MAX_LEN 83  // 79 chars + $ + CR + LF + null

// NMEA sentence generator for GPS emulation
class NMEAGenerator {
public:
    NMEAGenerator(NMEAGPSState& state, ISerialPort& serial);

    void setLogger(ILogger* logger) { _logger = logger; }

    // Output all sentences for one update cycle
    void outputAll();

    // Individual sentence output methods
    void outputGGA();   // GPS Fix Data
    void outputRMC();   // Recommended Minimum
    void outputGSA();   // DOP and Active Satellites
    void outputGSV();   // Satellites in View
    void outputVTG();   // Velocity Made Good

private:
    NMEAGPSState& _state;
    ISerialPort& _serial;
    ILogger* _logger;

    char _buffer[NMEA_SENTENCE_MAX_LEN];

    // Calculate NMEA checksum (XOR of all chars between $ and *)
    uint8_t calculateChecksum(const char* sentence);

    // Send a complete NMEA sentence with checksum
    void sendSentence(const char* sentence);

    // Format latitude to NMEA format (DDMM.MMMM)
    // buf must be at least 10 chars
    void formatLatitude(char* buf, double lat);

    // Format longitude to NMEA format (DDDMM.MMMM)
    // buf must be at least 11 chars
    void formatLongitude(char* buf, double lon);

    // Format time as HHMMSS.SS
    void formatTime(char* buf);

    // Format date as DDMMYY
    void formatDate(char* buf);
};
