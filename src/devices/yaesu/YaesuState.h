// Copyright (c) 2025 Andrew C. Young <andrew@vaelen.org>
// SPDX-License-Identifier: MIT

#pragma once

#include <Arduino.h>

// Yaesu operating modes
enum class YaesuMode : uint8_t {
    MODE_LSB = 1,
    MODE_USB = 2,
    MODE_CW_U = 3,
    MODE_FM = 4,
    MODE_AM = 5,
    MODE_RTTY_LSB = 6,
    MODE_CW_L = 7,
    MODE_DATA_LSB = 8,
    MODE_RTTY_USB = 9,
    MODE_DATA_FM = 10,
    MODE_FM_N = 11,
    MODE_DATA_USB = 12,
    MODE_AM_N = 13,
    MODE_C4FM = 14
};

// VFO selection
enum class YaesuVFO : uint8_t {
    VFO_A = 0,
    VFO_B = 1
};

// Radio ID for FT-991A
#define YAESU_RADIO_ID "0670"

// Default frequencies (Hz)
#define DEFAULT_FREQ_VFO_A 14074000UL   // 20m FT8
#define DEFAULT_FREQ_VFO_B 7074000UL    // 40m FT8

// Frequency limits (Hz)
#define FREQ_MIN 30000UL         // 30 kHz
#define FREQ_MAX 470000000UL     // 470 MHz

// State structure representing the emulated radio
struct YaesuState {
    // VFO frequencies (Hz)
    uint32_t freqVfoA;
    uint32_t freqVfoB;

    // Current VFO
    YaesuVFO currentVfo;

    // Operating mode for each VFO
    YaesuMode modeVfoA;
    YaesuMode modeVfoB;

    // PTT state
    bool ptt;

    // Power on/off
    bool powerOn;

    // RIT/XIT
    bool ritOn;
    bool xitOn;
    int16_t ritOffset;  // Hz, -9999 to +9999
    int16_t xitOffset;  // Hz, -9999 to +9999

    // Meters (console-controlled simulation values)
    uint8_t smeter;     // 0-255 (CAT reports 0-15 for S0-S9, or 0-255 for raw)
    uint8_t powerMeter; // 0-255
    uint8_t swrMeter;   // 0-255
    uint8_t alcMeter;   // 0-255
    uint8_t compMeter;  // 0-255

    // Squelch
    uint8_t squelch;    // 0-100

    // AF/RF gain
    uint8_t afGain;     // 0-255
    uint8_t rfGain;     // 0-255

    // Initialize to default values
    void reset() {
        freqVfoA = DEFAULT_FREQ_VFO_A;
        freqVfoB = DEFAULT_FREQ_VFO_B;
        currentVfo = YaesuVFO::VFO_A;
        modeVfoA = YaesuMode::MODE_USB;
        modeVfoB = YaesuMode::MODE_USB;
        ptt = false;
        powerOn = true;
        ritOn = false;
        xitOn = false;
        ritOffset = 0;
        xitOffset = 0;
        smeter = 0;
        powerMeter = 0;
        swrMeter = 0;
        alcMeter = 0;
        compMeter = 0;
        squelch = 50;
        afGain = 128;
        rfGain = 255;
    }

    // Get frequency of current VFO
    uint32_t getCurrentFreq() const {
        return (currentVfo == YaesuVFO::VFO_A) ? freqVfoA : freqVfoB;
    }

    // Set frequency of current VFO
    void setCurrentFreq(uint32_t freq) {
        if (currentVfo == YaesuVFO::VFO_A) {
            freqVfoA = freq;
        } else {
            freqVfoB = freq;
        }
    }

    // Get mode of current VFO
    YaesuMode getCurrentMode() const {
        return (currentVfo == YaesuVFO::VFO_A) ? modeVfoA : modeVfoB;
    }

    // Set mode of current VFO
    void setCurrentMode(YaesuMode mode) {
        if (currentVfo == YaesuVFO::VFO_A) {
            modeVfoA = mode;
        } else {
            modeVfoB = mode;
        }
    }
};
