// Copyright (c) 2025 Andrew C. Young <andrew@vaelen.org>
// SPDX-License-Identifier: MIT

#pragma once

#include <Arduino.h>

// Default position (San Francisco, CA)
#define DEFAULT_LATITUDE 37.7749
#define DEFAULT_LONGITUDE -122.4194
#define DEFAULT_ALTITUDE 10.0f

// Default fix parameters
#define DEFAULT_FIX_QUALITY 1      // GPS fix
#define DEFAULT_NUM_SATELLITES 8
#define DEFAULT_HDOP 1.0f
#define DEFAULT_SPEED_KNOTS 0.0f
#define DEFAULT_COURSE 0.0f

// Simulated satellite data (PRN numbers)
#define MAX_SATELLITES 12

// GPS state structure
struct NMEAGPSState {
    // Position (decimal degrees)
    double latitude;      // Positive = North, Negative = South
    double longitude;     // Positive = East, Negative = West
    float altitude;       // Meters above mean sea level
    float geoidSep;       // Geoid separation (meters)

    // Velocity
    float speedKnots;     // Speed over ground in knots
    float courseTrue;     // Course over ground (degrees true north)
    float courseMag;      // Course over ground (degrees magnetic)

    // Fix information
    uint8_t fixQuality;   // 0=invalid, 1=GPS fix, 2=DGPS fix
    uint8_t fixMode;      // 1=no fix, 2=2D, 3=3D
    uint8_t numSatellites;

    // Dilution of precision
    float pdop;           // Position DOP
    float hdop;           // Horizontal DOP
    float vdop;           // Vertical DOP

    // Simulated satellite info
    uint8_t satPRN[MAX_SATELLITES];       // Satellite PRN numbers
    uint8_t satElevation[MAX_SATELLITES]; // Elevation (degrees)
    uint16_t satAzimuth[MAX_SATELLITES];  // Azimuth (degrees)
    uint8_t satSNR[MAX_SATELLITES];       // Signal-to-noise ratio (dB-Hz)
    uint8_t numSatsInView;

    // Simulated UTC time
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t day;
    uint8_t month;
    uint16_t year;

    // Magnetic variation (degrees, positive = East)
    float magVariation;

    // Output timing
    unsigned long lastOutputMs;

    // Initialize to default values
    void reset() {
        latitude = DEFAULT_LATITUDE;
        longitude = DEFAULT_LONGITUDE;
        altitude = DEFAULT_ALTITUDE;
        geoidSep = -34.0f;  // Typical value for SF area

        speedKnots = DEFAULT_SPEED_KNOTS;
        courseTrue = DEFAULT_COURSE;
        courseMag = DEFAULT_COURSE;

        fixQuality = DEFAULT_FIX_QUALITY;
        fixMode = 3;  // 3D fix
        numSatellites = DEFAULT_NUM_SATELLITES;

        pdop = 1.5f;
        hdop = DEFAULT_HDOP;
        vdop = 1.2f;

        // Initialize simulated satellites
        initSatellites();

        // Set initial time to midnight UTC, Jan 1, 2025
        hour = 12;
        minute = 0;
        second = 0;
        day = 1;
        month = 1;
        year = 2025;

        magVariation = 13.0f;  // Typical for SF area (East)

        lastOutputMs = 0;
    }

    // Initialize simulated satellite constellation
    void initSatellites() {
        numSatsInView = 8;

        // Simulated satellite data (PRN, elevation, azimuth, SNR)
        // Using uint16_t to accommodate azimuth values > 255
        const uint16_t satData[][4] = {
            {2,  45, 120, 42},
            {5,  67, 230, 45},
            {9,  23, 45,  38},
            {12, 34, 315, 40},
            {15, 56, 180, 44},
            {18, 12, 90,  35},
            {21, 78, 270, 47},
            {25, 41, 150, 41}
        };

        for (uint8_t i = 0; i < numSatsInView && i < MAX_SATELLITES; i++) {
            satPRN[i] = (uint8_t)satData[i][0];
            satElevation[i] = (uint8_t)satData[i][1];
            satAzimuth[i] = satData[i][2];
            satSNR[i] = (uint8_t)satData[i][3];
        }

        // Fill rest with zeros
        for (uint8_t i = numSatsInView; i < MAX_SATELLITES; i++) {
            satPRN[i] = 0;
            satElevation[i] = 0;
            satAzimuth[i] = 0;
            satSNR[i] = 0;
        }
    }

    // Advance simulated time by one second
    void advanceTime() {
        second++;
        if (second >= 60) {
            second = 0;
            minute++;
            if (minute >= 60) {
                minute = 0;
                hour++;
                if (hour >= 24) {
                    hour = 0;
                    day++;
                    // Simple month handling (doesn't account for varying month lengths)
                    if (day > 28) {
                        day = 1;
                        month++;
                        if (month > 12) {
                            month = 1;
                            year++;
                        }
                    }
                }
            }
        }
    }

    // Set position from decimal degrees
    void setPosition(double lat, double lon, float alt = 0.0f) {
        latitude = lat;
        longitude = lon;
        altitude = alt;
    }

    // Set simulated UTC time and optionally date
    void setTime(uint8_t h, uint8_t m, uint8_t s,
                 uint8_t d = 0, uint8_t mo = 0, uint16_t y = 0) {
        hour = h;
        minute = m;
        second = s;
        if (d > 0) day = d;
        if (mo > 0) month = mo;
        if (y > 0) year = y;
    }

    // Check if we have a valid fix
    bool hasValidFix() const {
        return fixQuality > 0;
    }

    // Get latitude hemisphere character
    char getLatHemisphere() const {
        return latitude >= 0 ? 'N' : 'S';
    }

    // Get longitude hemisphere character
    char getLonHemisphere() const {
        return longitude >= 0 ? 'E' : 'W';
    }
};
