// Copyright (c) 2025 Andrew C. Young <andrew@vaelen.org>
// SPDX-License-Identifier: MIT

#pragma once

#include <Arduino.h>

// Azimuth range (degrees)
// G-5500 supports 0-450 degrees to allow overlap at north
#define AZ_MIN 0.0f
#define AZ_MAX 450.0f

// Elevation range (degrees)
#define EL_MIN 0.0f
#define EL_MAX 180.0f

// Default rotation speeds (degrees per second)
// Real G-5500: ~1 RPM azimuth = 6 deg/sec, ~1 deg/sec elevation
#define DEFAULT_AZ_SPEED 2.0f
#define DEFAULT_EL_SPEED 1.0f

// Rotation direction
enum class RotationDir : uint8_t {
    STOPPED = 0,
    CW,      // Clockwise (azimuth increase) or Up (elevation increase)
    CCW      // Counter-clockwise (azimuth decrease) or Down (elevation decrease)
};

// G-5500 rotator state structure
struct G5500State {
    // Current position (degrees, with decimal precision for smooth simulation)
    float azimuth;
    float elevation;

    // Target position for goto commands (M and W commands)
    float targetAzimuth;
    float targetElevation;

    // Rotation status
    RotationDir azRotation;
    RotationDir elRotation;

    // Whether we're in "goto" mode (moving to target) vs manual rotation
    bool azGotoMode;
    bool elGotoMode;

    // Timestamp of last update (for calculating movement)
    unsigned long lastUpdateMs;

    // Initialize to default values
    void reset() {
        azimuth = 0.0f;
        elevation = 0.0f;
        targetAzimuth = 0.0f;
        targetElevation = 0.0f;
        azRotation = RotationDir::STOPPED;
        elRotation = RotationDir::STOPPED;
        azGotoMode = false;
        elGotoMode = false;
        lastUpdateMs = 0;
    }

    // Get azimuth as integer (0-450) for protocol responses
    int getAzimuthInt() const {
        return (int)(azimuth + 0.5f);
    }

    // Get elevation as integer (0-180) for protocol responses
    int getElevationInt() const {
        return (int)(elevation + 0.5f);
    }

    // Check if azimuth rotation is in progress
    bool isAzimuthMoving() const {
        return azRotation != RotationDir::STOPPED;
    }

    // Check if elevation rotation is in progress
    bool isElevationMoving() const {
        return elRotation != RotationDir::STOPPED;
    }

    // Check if any rotation is in progress
    bool isMoving() const {
        return isAzimuthMoving() || isElevationMoving();
    }

    // Stop azimuth rotation
    void stopAzimuth() {
        azRotation = RotationDir::STOPPED;
        azGotoMode = false;
    }

    // Stop elevation rotation
    void stopElevation() {
        elRotation = RotationDir::STOPPED;
        elGotoMode = false;
    }

    // Stop all rotation
    void stopAll() {
        stopAzimuth();
        stopElevation();
    }

    // Start manual CW rotation (azimuth increase)
    void rotateCW() {
        azRotation = RotationDir::CW;
        azGotoMode = false;
    }

    // Start manual CCW rotation (azimuth decrease)
    void rotateCCW() {
        azRotation = RotationDir::CCW;
        azGotoMode = false;
    }

    // Start manual up rotation (elevation increase)
    void rotateUp() {
        elRotation = RotationDir::CW;
        elGotoMode = false;
    }

    // Start manual down rotation (elevation decrease)
    void rotateDown() {
        elRotation = RotationDir::CCW;
        elGotoMode = false;
    }

    // Start goto azimuth (move to target)
    void gotoAzimuth(float target) {
        targetAzimuth = constrain(target, AZ_MIN, AZ_MAX);
        azGotoMode = true;
        // Determine direction based on shortest path
        if (targetAzimuth > azimuth) {
            azRotation = RotationDir::CW;
        } else if (targetAzimuth < azimuth) {
            azRotation = RotationDir::CCW;
        } else {
            azRotation = RotationDir::STOPPED;
            azGotoMode = false;
        }
    }

    // Start goto elevation (move to target)
    void gotoElevation(float target) {
        targetElevation = constrain(target, EL_MIN, EL_MAX);
        elGotoMode = true;
        if (targetElevation > elevation) {
            elRotation = RotationDir::CW;
        } else if (targetElevation < elevation) {
            elRotation = RotationDir::CCW;
        } else {
            elRotation = RotationDir::STOPPED;
            elGotoMode = false;
        }
    }
};
