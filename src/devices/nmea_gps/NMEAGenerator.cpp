// Copyright (c) 2025 Andrew C. Young <andrew@vaelen.org>
// SPDX-License-Identifier: MIT

#include "NMEAGenerator.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Helper to format float (printf %f not supported on all platforms)
static char* fmtFloat(char* buf, double val, int width, int prec) {
    dtostrf(val, width, prec, buf);
    return buf;
}

NMEAGenerator::NMEAGenerator(NMEAGPSState& state, ISerialPort& serial)
    : _state(state)
    , _serial(serial)
    , _logger(nullptr)
{
    memset(_buffer, 0, sizeof(_buffer));
}

void NMEAGenerator::outputAll() {
    outputGGA();
    outputRMC();
    outputGSA();
    outputGSV();
    outputVTG();
}

uint8_t NMEAGenerator::calculateChecksum(const char* sentence) {
    uint8_t checksum = 0;

    // Skip the leading '$' if present
    if (*sentence == '$') {
        sentence++;
    }

    // XOR all characters until '*' or end of string
    while (*sentence && *sentence != '*') {
        checksum ^= *sentence++;
    }

    return checksum;
}

void NMEAGenerator::sendSentence(const char* sentence) {
    uint8_t checksum = calculateChecksum(sentence);

    // Build complete sentence with checksum
    char finalSentence[NMEA_SENTENCE_MAX_LEN];
    snprintf(finalSentence, sizeof(finalSentence), "%s*%02X\r\n", sentence, checksum);

    _serial.print(finalSentence);

    if (_logger) {
        // Remove CR LF for logging
        size_t len = strlen(finalSentence);
        if (len >= 2) {
            finalSentence[len - 2] = '\0';
        }
        _logger->logf(LogLevel::DEBUG, "NMEA", "TX: %s", finalSentence);
    }
}

void NMEAGenerator::formatLatitude(char* buf, double lat) {
    // Convert decimal degrees to DDMM.MMMM format
    double absLat = fabs(lat);
    int degrees = (int)absLat;
    double minutes = (absLat - degrees) * 60.0;

    // Format minutes with leading zero if needed (07.4 format = 7 chars, 4 decimals)
    char minBuf[12];
    dtostrf(minutes, 7, 4, minBuf);
    // Replace leading spaces with zeros for NMEA format
    for (int i = 0; minBuf[i] == ' '; i++) minBuf[i] = '0';
    snprintf(buf, 12, "%02d%s", degrees, minBuf);
}

void NMEAGenerator::formatLongitude(char* buf, double lon) {
    // Convert decimal degrees to DDDMM.MMMM format
    double absLon = fabs(lon);
    int degrees = (int)absLon;
    double minutes = (absLon - degrees) * 60.0;

    // Format minutes with leading zero if needed (07.4 format = 7 chars, 4 decimals)
    char minBuf[12];
    dtostrf(minutes, 7, 4, minBuf);
    // Replace leading spaces with zeros for NMEA format
    for (int i = 0; minBuf[i] == ' '; i++) minBuf[i] = '0';
    snprintf(buf, 12, "%03d%s", degrees, minBuf);
}

void NMEAGenerator::formatTime(char* buf) {
    snprintf(buf, 11, "%02d%02d%02d.00",
             _state.hour, _state.minute, _state.second);
}

void NMEAGenerator::formatDate(char* buf) {
    snprintf(buf, 7, "%02d%02d%02d",
             _state.day, _state.month, _state.year % 100);
}

// GGA - GPS Fix Data
// $GPGGA,HHMMSS.SS,DDMM.MMMM,N,DDDMM.MMMM,E,Q,NN,H.H,AAA.A,M,GGG.G,M,,*CC
void NMEAGenerator::outputGGA() {
    char time[12];
    char lat[12];
    char lon[12];
    char hdopStr[8], altStr[12], geoidStr[12];

    formatTime(time);
    formatLatitude(lat, _state.latitude);
    formatLongitude(lon, _state.longitude);

    snprintf(_buffer, sizeof(_buffer),
             "$GPGGA,%s,%s,%c,%s,%c,%d,%02d,%s,%s,M,%s,M,,",
             time,
             lat, _state.getLatHemisphere(),
             lon, _state.getLonHemisphere(),
             _state.fixQuality,
             _state.numSatellites,
             fmtFloat(hdopStr, _state.hdop, 1, 1),
             fmtFloat(altStr, _state.altitude, 1, 1),
             fmtFloat(geoidStr, _state.geoidSep, 1, 1));

    sendSentence(_buffer);
}

// RMC - Recommended Minimum Navigation Information
// $GPRMC,HHMMSS.SS,A,DDMM.MMMM,N,DDDMM.MMMM,E,SSS.S,CCC.C,DDMMYY,VAR,D*CC
void NMEAGenerator::outputRMC() {
    char time[12];
    char date[8];
    char lat[12];
    char lon[12];
    char speedStr[12], courseStr[12], magVarStr[12];

    formatTime(time);
    formatDate(date);
    formatLatitude(lat, _state.latitude);
    formatLongitude(lon, _state.longitude);

    char status = _state.hasValidFix() ? 'A' : 'V';
    char magDir = _state.magVariation >= 0 ? 'E' : 'W';

    snprintf(_buffer, sizeof(_buffer),
             "$GPRMC,%s,%c,%s,%c,%s,%c,%s,%s,%s,%s,%c,A",
             time,
             status,
             lat, _state.getLatHemisphere(),
             lon, _state.getLonHemisphere(),
             fmtFloat(speedStr, _state.speedKnots, 1, 1),
             fmtFloat(courseStr, _state.courseTrue, 1, 1),
             date,
             fmtFloat(magVarStr, fabs(_state.magVariation), 1, 1), magDir);

    sendSentence(_buffer);
}

// GSA - GPS DOP and Active Satellites
// $GPGSA,A,3,04,05,,09,12,,,24,,,,,2.5,1.3,2.1*39
void NMEAGenerator::outputGSA() {
    // Start building the sentence
    int pos = snprintf(_buffer, sizeof(_buffer), "$GPGSA,A,%d", _state.fixMode);

    // Add up to 12 satellite PRNs (or empty fields)
    for (int i = 0; i < 12; i++) {
        if (i < _state.numSatsInView && _state.satPRN[i] > 0) {
            pos += snprintf(_buffer + pos, sizeof(_buffer) - pos, ",%02d", _state.satPRN[i]);
        } else {
            pos += snprintf(_buffer + pos, sizeof(_buffer) - pos, ",");
        }
    }

    // Add DOP values
    char pdopStr[8], hdopStr[8], vdopStr[8];
    snprintf(_buffer + pos, sizeof(_buffer) - pos, ",%s,%s,%s",
             fmtFloat(pdopStr, _state.pdop, 1, 1),
             fmtFloat(hdopStr, _state.hdop, 1, 1),
             fmtFloat(vdopStr, _state.vdop, 1, 1));

    sendSentence(_buffer);
}

// GSV - Satellites in View
// $GPGSV,2,1,08,01,40,083,46,02,17,308,41,12,07,344,39,14,22,228,45*75
void NMEAGenerator::outputGSV() {
    // Calculate number of messages needed (4 satellites per message)
    int numMsgs = (_state.numSatsInView + 3) / 4;
    if (numMsgs == 0) numMsgs = 1;

    int satIdx = 0;

    for (int msg = 1; msg <= numMsgs; msg++) {
        int pos = snprintf(_buffer, sizeof(_buffer), "$GPGSV,%d,%d,%02d",
                          numMsgs, msg, _state.numSatsInView);

        // Add up to 4 satellites per message
        for (int i = 0; i < 4 && satIdx < _state.numSatsInView; i++, satIdx++) {
            pos += snprintf(_buffer + pos, sizeof(_buffer) - pos,
                           ",%02d,%02d,%03d,%02d",
                           _state.satPRN[satIdx],
                           _state.satElevation[satIdx],
                           _state.satAzimuth[satIdx],
                           _state.satSNR[satIdx]);
        }

        // Pad remaining satellite fields if needed (for last message)
        int remaining = 4 - (satIdx % 4);
        if (msg == numMsgs && remaining < 4) {
            // Don't pad - we only output what we have
        }

        sendSentence(_buffer);
    }
}

// VTG - Velocity Made Good
// $GPVTG,054.7,T,034.4,M,005.5,N,010.2,K*48
void NMEAGenerator::outputVTG() {
    // Convert knots to km/h
    float speedKmh = _state.speedKnots * 1.852f;

    char courseTStr[12], courseMStr[12], speedKnStr[12], speedKmStr[12];
    snprintf(_buffer, sizeof(_buffer),
             "$GPVTG,%s,T,%s,M,%s,N,%s,K,A",
             fmtFloat(courseTStr, _state.courseTrue, 1, 1),
             fmtFloat(courseMStr, _state.courseMag, 1, 1),
             fmtFloat(speedKnStr, _state.speedKnots, 1, 1),
             fmtFloat(speedKmStr, speedKmh, 1, 1));

    sendSentence(_buffer);
}
