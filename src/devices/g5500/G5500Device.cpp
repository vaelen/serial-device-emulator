// Copyright (c) 2025 Andrew C. Young <andrew@vaelen.org>
// SPDX-License-Identifier: MIT

#include "G5500Device.h"
#include <string.h>
#include <stdio.h>

// Baud rate options for GS-232
static const char* BAUD_RATE_OPTIONS[] = {"1200", "4800", "9600"};
static const uint32_t BAUD_RATE_VALUES[] = {1200, 4800, 9600};
static const size_t NUM_BAUD_RATES = 3;
static const uint8_t DEFAULT_BAUD_INDEX = 2;  // 9600 baud default

// Speed range (degrees per second)
static const uint32_t MIN_SPEED = 1;
static const uint32_t MAX_SPEED = 10;
static const uint32_t DEFAULT_AZ_SPEED_INT = 2;
static const uint32_t DEFAULT_EL_SPEED_INT = 1;

// Minimum update interval (ms) to prevent jitter
static const unsigned long MIN_UPDATE_INTERVAL = 10;

G5500Device::G5500Device(ISerialPort* serial, uint8_t uartIndex)
    : _serial(serial)
    , _uartIndex(uartIndex)
    , _deviceId(0xFF)
    , _running(false)
    , _logger(nullptr)
    , _parser(_state, *serial)
{
    _state.reset();
    initOptions();
}

G5500Device::~G5500Device() {
    if (_running) {
        end();
    }
}

void G5500Device::initOptions() {
    // Option 0: Baud rate
    _options[0] = makeEnumOption(
        "baud_rate",
        "Serial baud rate",
        BAUD_RATE_OPTIONS,
        NUM_BAUD_RATES,
        DEFAULT_BAUD_INDEX
    );

    // Option 1: Azimuth rotation speed (degrees per second)
    _options[1] = makeUint32Option(
        "az_speed",
        "Azimuth speed (deg/sec)",
        MIN_SPEED,
        MAX_SPEED,
        DEFAULT_AZ_SPEED_INT
    );

    // Option 2: Elevation rotation speed (degrees per second)
    _options[2] = makeUint32Option(
        "el_speed",
        "Elevation speed (deg/sec)",
        MIN_SPEED,
        MAX_SPEED,
        DEFAULT_EL_SPEED_INT
    );
}

bool G5500Device::begin() {
    if (_serial == nullptr) {
        return false;
    }

    applyBaudRate();
    _parser.reset();
    _state.reset();
    _state.lastUpdateMs = millis();
    _running = true;

    if (_logger) {
        uint32_t baud = BAUD_RATE_VALUES[_options[0].value.enumVal.current];
        _logger->logf(LogLevel::INFO, "G5500", "Started on UART %d at %lu baud", _uartIndex, baud);
    }

    return true;
}

void G5500Device::end() {
    _running = false;
    _state.stopAll();
    _serial->end();

    if (_logger) {
        _logger->logf(LogLevel::INFO, "G5500", "Stopped on UART %d", _uartIndex);
    }
}

void G5500Device::update() {
    if (!_running) return;

    // Process incoming GS-232 commands
    _parser.update();

    // Simulate rotation based on elapsed time
    simulateRotation();
}

void G5500Device::simulateRotation() {
    unsigned long now = millis();
    unsigned long deltaMs = now - _state.lastUpdateMs;

    // Limit update rate to prevent jitter
    if (deltaMs < MIN_UPDATE_INTERVAL) {
        return;
    }

    _state.lastUpdateMs = now;
    float deltaSec = deltaMs / 1000.0f;

    // Get configured speeds
    float azSpeed = getAzSpeed();
    float elSpeed = getElSpeed();

    // Update azimuth position
    if (_state.azRotation != RotationDir::STOPPED) {
        float azDelta = azSpeed * deltaSec;

        if (_state.azRotation == RotationDir::CW) {
            _state.azimuth += azDelta;

            // Check if we've reached target in goto mode
            if (_state.azGotoMode && _state.azimuth >= _state.targetAzimuth) {
                _state.azimuth = _state.targetAzimuth;
                _state.stopAzimuth();
            }

            // Clamp to max
            if (_state.azimuth > AZ_MAX) {
                _state.azimuth = AZ_MAX;
                _state.stopAzimuth();
            }
        } else {
            _state.azimuth -= azDelta;

            // Check if we've reached target in goto mode
            if (_state.azGotoMode && _state.azimuth <= _state.targetAzimuth) {
                _state.azimuth = _state.targetAzimuth;
                _state.stopAzimuth();
            }

            // Clamp to min
            if (_state.azimuth < AZ_MIN) {
                _state.azimuth = AZ_MIN;
                _state.stopAzimuth();
            }
        }
    }

    // Update elevation position
    if (_state.elRotation != RotationDir::STOPPED) {
        float elDelta = elSpeed * deltaSec;

        if (_state.elRotation == RotationDir::CW) {
            _state.elevation += elDelta;

            // Check if we've reached target in goto mode
            if (_state.elGotoMode && _state.elevation >= _state.targetElevation) {
                _state.elevation = _state.targetElevation;
                _state.stopElevation();
            }

            // Clamp to max
            if (_state.elevation > EL_MAX) {
                _state.elevation = EL_MAX;
                _state.stopElevation();
            }
        } else {
            _state.elevation -= elDelta;

            // Check if we've reached target in goto mode
            if (_state.elGotoMode && _state.elevation <= _state.targetElevation) {
                _state.elevation = _state.targetElevation;
                _state.stopElevation();
            }

            // Clamp to min
            if (_state.elevation < EL_MIN) {
                _state.elevation = EL_MIN;
                _state.stopElevation();
            }
        }
    }
}

void G5500Device::applyBaudRate() {
    uint8_t baudIndex = _options[0].value.enumVal.current;
    if (baudIndex >= NUM_BAUD_RATES) {
        baudIndex = DEFAULT_BAUD_INDEX;
    }
    uint32_t baud = BAUD_RATE_VALUES[baudIndex];
    _serial->begin(baud);
}

float G5500Device::getAzSpeed() const {
    return (float)_options[1].value.uint32Val.current;
}

float G5500Device::getElSpeed() const {
    return (float)_options[2].value.uint32Val.current;
}

const DeviceOption* G5500Device::getOption(size_t index) const {
    if (index >= G5500_OPTION_COUNT) {
        return nullptr;
    }
    return &_options[index];
}

DeviceOption* G5500Device::findOption(const char* name) {
    for (size_t i = 0; i < G5500_OPTION_COUNT; i++) {
        if (strcmp(_options[i].name, name) == 0) {
            return &_options[i];
        }
    }
    return nullptr;
}

bool G5500Device::setOption(const char* name, const char* value) {
    DeviceOption* opt = findOption(name);
    if (opt == nullptr) {
        return false;
    }

    if (!parseOptionValue(*opt, value)) {
        return false;
    }

    // Apply baud rate change immediately if running
    if (strcmp(name, "baud_rate") == 0 && _running) {
        applyBaudRate();
    }

    return true;
}

bool G5500Device::getOptionValue(const char* name, char* buffer, size_t bufLen) const {
    for (size_t i = 0; i < G5500_OPTION_COUNT; i++) {
        if (strcmp(_options[i].name, name) == 0) {
            formatOptionValue(_options[i], buffer, bufLen);
            return true;
        }
    }
    return false;
}

size_t G5500Device::serializeOptions(uint8_t* buffer, size_t bufLen) const {
    // Format: [baud_index (1 byte)][az_speed (1 byte)][el_speed (1 byte)]
    if (bufLen < 3) {
        return 0;
    }

    buffer[0] = _options[0].value.enumVal.current;  // Baud rate index
    buffer[1] = (uint8_t)_options[1].value.uint32Val.current;  // Azimuth speed
    buffer[2] = (uint8_t)_options[2].value.uint32Val.current;  // Elevation speed

    return 3;
}

bool G5500Device::deserializeOptions(const uint8_t* buffer, size_t len) {
    if (len < 3) {
        return false;
    }

    // Validate and restore baud rate
    uint8_t baudIndex = buffer[0];
    if (baudIndex >= NUM_BAUD_RATES) {
        baudIndex = DEFAULT_BAUD_INDEX;
    }
    _options[0].value.enumVal.current = baudIndex;

    // Validate and restore azimuth speed
    uint32_t azSpeed = buffer[1];
    if (azSpeed < MIN_SPEED || azSpeed > MAX_SPEED) {
        azSpeed = DEFAULT_AZ_SPEED_INT;
    }
    _options[1].value.uint32Val.current = azSpeed;

    // Validate and restore elevation speed
    uint32_t elSpeed = buffer[2];
    if (elSpeed < MIN_SPEED || elSpeed > MAX_SPEED) {
        elSpeed = DEFAULT_EL_SPEED_INT;
    }
    _options[2].value.uint32Val.current = elSpeed;

    return true;
}

bool G5500Device::setMeter(MeterType type, uint8_t value) {
    // Meters not applicable for rotators
    (void)type;
    (void)value;
    return false;
}

uint8_t G5500Device::getMeter(MeterType type) const {
    // Meters not applicable for rotators
    (void)type;
    return 0;
}

void G5500Device::setLogger(ILogger* logger) {
    _logger = logger;
    _parser.setLogger(logger);
}

void G5500Device::getStatus(char* buffer, size_t bufLen) const {
    const char* azStatus = "stopped";
    const char* elStatus = "stopped";

    if (_state.azRotation == RotationDir::CW) {
        azStatus = _state.azGotoMode ? "goto CW" : "CW";
    } else if (_state.azRotation == RotationDir::CCW) {
        azStatus = _state.azGotoMode ? "goto CCW" : "CCW";
    }

    if (_state.elRotation == RotationDir::CW) {
        elStatus = _state.elGotoMode ? "goto UP" : "UP";
    } else if (_state.elRotation == RotationDir::CCW) {
        elStatus = _state.elGotoMode ? "goto DOWN" : "DOWN";
    }

    snprintf(buffer, bufLen,
             "  Azimuth: %d deg (%s)\n"
             "  Elevation: %d deg (%s)\n"
             "  Target Az: %d deg\n"
             "  Target El: %d deg\n"
             "  Az Speed: %lu deg/sec\n"
             "  El Speed: %lu deg/sec",
             _state.getAzimuthInt(), azStatus,
             _state.getElevationInt(), elStatus,
             (int)_state.targetAzimuth,
             (int)_state.targetElevation,
             (unsigned long)_options[1].value.uint32Val.current,
             (unsigned long)_options[2].value.uint32Val.current);
}

// === Factory Implementation ===

IEmulatedDevice* G5500DeviceFactory::create(ISerialPort* serial, uint8_t uartIndex) {
    return new G5500Device(serial, uartIndex);
}

void G5500DeviceFactory::destroy(IEmulatedDevice* device) {
    delete device;
}
