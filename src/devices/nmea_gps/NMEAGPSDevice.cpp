// Copyright (c) 2025 Andrew C. Young <andrew@vaelen.org>
// SPDX-License-Identifier: MIT

#include "NMEAGPSDevice.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Helper to format float (printf %f not supported on all platforms)
static char* fmtFloat(char* buf, double val, int width, int prec) {
    dtostrf(val, width, prec, buf);
    return buf;
}

// Baud rate options for NMEA GPS
static const char* BAUD_RATE_OPTIONS[] = {"4800", "9600", "19200", "38400"};
static const uint32_t BAUD_RATE_VALUES[] = {4800, 9600, 19200, 38400};
static const size_t NUM_BAUD_RATES = 4;
static const uint8_t DEFAULT_BAUD_INDEX = 1;  // 9600 baud default

// Update rate options (Hz)
static const char* UPDATE_RATE_OPTIONS[] = {"1", "5", "10"};
static const uint32_t UPDATE_RATE_VALUES[] = {1, 5, 10};
static const size_t NUM_UPDATE_RATES = 3;
static const uint8_t DEFAULT_RATE_INDEX = 0;  // 1 Hz default

NMEAGPSDevice::NMEAGPSDevice(ISerialPort* serial, uint8_t uartIndex)
    : _serial(serial)
    , _uartIndex(uartIndex)
    , _deviceId(0xFF)
    , _running(false)
    , _logger(nullptr)
    , _generator(_state, *serial)
{
    _state.reset();
    initOptions();
}

NMEAGPSDevice::~NMEAGPSDevice() {
    if (_running) {
        end();
    }
}

void NMEAGPSDevice::initOptions() {
    // Option 0: Baud rate
    _options[0] = makeEnumOption(
        "baud_rate",
        "Serial baud rate",
        BAUD_RATE_OPTIONS,
        NUM_BAUD_RATES,
        DEFAULT_BAUD_INDEX
    );

    // Option 1: Update rate (Hz)
    _options[1] = makeEnumOption(
        "update_rate",
        "Output rate (Hz)",
        UPDATE_RATE_OPTIONS,
        NUM_UPDATE_RATES,
        DEFAULT_RATE_INDEX
    );
}

bool NMEAGPSDevice::begin() {
    if (_serial == nullptr) {
        return false;
    }

    applyBaudRate();
    _state.reset();
    _state.lastOutputMs = millis();
    _running = true;

    if (_logger) {
        uint32_t baud = BAUD_RATE_VALUES[_options[0].value.enumVal.current];
        uint32_t rate = UPDATE_RATE_VALUES[_options[1].value.enumVal.current];
        _logger->logf(LogLevel::INFO, "NMEA", "Started on UART %d at %lu baud, %lu Hz",
                      _uartIndex, baud, rate);
    }

    return true;
}

void NMEAGPSDevice::end() {
    _running = false;
    _serial->end();

    if (_logger) {
        _logger->logf(LogLevel::INFO, "NMEA", "Stopped on UART %d", _uartIndex);
    }
}

void NMEAGPSDevice::update() {
    if (!_running) return;

    unsigned long now = millis();
    unsigned long interval = getUpdateIntervalMs();

    if (now - _state.lastOutputMs >= interval) {
        _state.lastOutputMs = now;

        // Advance simulated time
        _state.advanceTime();

        // Output all NMEA sentences
        _generator.outputAll();
    }
}

unsigned long NMEAGPSDevice::getUpdateIntervalMs() const {
    uint8_t rateIndex = _options[1].value.enumVal.current;
    if (rateIndex >= NUM_UPDATE_RATES) {
        rateIndex = DEFAULT_RATE_INDEX;
    }
    uint32_t hz = UPDATE_RATE_VALUES[rateIndex];
    return 1000 / hz;
}

void NMEAGPSDevice::applyBaudRate() {
    uint8_t baudIndex = _options[0].value.enumVal.current;
    if (baudIndex >= NUM_BAUD_RATES) {
        baudIndex = DEFAULT_BAUD_INDEX;
    }
    uint32_t baud = BAUD_RATE_VALUES[baudIndex];
    _serial->begin(baud);
}

const DeviceOption* NMEAGPSDevice::getOption(size_t index) const {
    if (index >= NMEA_GPS_OPTION_COUNT) {
        return nullptr;
    }
    return &_options[index];
}

DeviceOption* NMEAGPSDevice::findOption(const char* name) {
    for (size_t i = 0; i < NMEA_GPS_OPTION_COUNT; i++) {
        if (strcmp(_options[i].name, name) == 0) {
            return &_options[i];
        }
    }
    return nullptr;
}

bool NMEAGPSDevice::setOption(const char* name, const char* value) {
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

bool NMEAGPSDevice::getOptionValue(const char* name, char* buffer, size_t bufLen) const {
    for (size_t i = 0; i < NMEA_GPS_OPTION_COUNT; i++) {
        if (strcmp(_options[i].name, name) == 0) {
            formatOptionValue(_options[i], buffer, bufLen);
            return true;
        }
    }
    return false;
}

size_t NMEAGPSDevice::serializeOptions(uint8_t* buffer, size_t bufLen) const {
    // Format: [baud_index (1 byte)][rate_index (1 byte)]
    if (bufLen < 2) {
        return 0;
    }

    buffer[0] = _options[0].value.enumVal.current;  // Baud rate index
    buffer[1] = _options[1].value.enumVal.current;  // Update rate index

    return 2;
}

bool NMEAGPSDevice::deserializeOptions(const uint8_t* buffer, size_t len) {
    if (len < 2) {
        return false;
    }

    // Validate and restore baud rate
    uint8_t baudIndex = buffer[0];
    if (baudIndex >= NUM_BAUD_RATES) {
        baudIndex = DEFAULT_BAUD_INDEX;
    }
    _options[0].value.enumVal.current = baudIndex;

    // Validate and restore update rate
    uint8_t rateIndex = buffer[1];
    if (rateIndex >= NUM_UPDATE_RATES) {
        rateIndex = DEFAULT_RATE_INDEX;
    }
    _options[1].value.enumVal.current = rateIndex;

    return true;
}

bool NMEAGPSDevice::setMeter(MeterType type, uint8_t value) {
    // Meters not applicable for GPS
    (void)type;
    (void)value;
    return false;
}

uint8_t NMEAGPSDevice::getMeter(MeterType type) const {
    // Meters not applicable for GPS
    (void)type;
    return 0;
}

void NMEAGPSDevice::setLogger(ILogger* logger) {
    _logger = logger;
    _generator.setLogger(logger);
}

void NMEAGPSDevice::setPosition(double lat, double lon, float alt) {
    _state.setPosition(lat, lon, alt);

    if (_logger) {
        char latStr[16], lonStr[16], altStr[12];
        _logger->logf(LogLevel::INFO, "NMEA", "Position set to %s, %s, %sm",
                      fmtFloat(latStr, lat, 1, 6),
                      fmtFloat(lonStr, lon, 1, 6),
                      fmtFloat(altStr, alt, 1, 1));
    }
}

void NMEAGPSDevice::getStatus(char* buffer, size_t bufLen) const {
    const char* fixStatus = "No fix";
    if (_state.fixQuality == 1) fixStatus = "GPS fix";
    else if (_state.fixQuality == 2) fixStatus = "DGPS fix";

    uint32_t rate = UPDATE_RATE_VALUES[_options[1].value.enumVal.current];

    char latStr[16], lonStr[16], altStr[12], speedStr[12], courseStr[12], hdopStr[8];
    snprintf(buffer, bufLen,
             "  Position: %s, %s\r\n"
             "  Altitude: %s m\r\n"
             "  Speed: %s knots\r\n"
             "  Course: %s deg\r\n"
             "  Fix: %s (%d satellites)\r\n"
             "  HDOP: %s\r\n"
             "  Time: %02d:%02d:%02d UTC\r\n"
             "  Date: %04d-%02d-%02d\r\n"
             "  Update rate: %lu Hz",
             fmtFloat(latStr, _state.latitude, 1, 6),
             fmtFloat(lonStr, _state.longitude, 1, 6),
             fmtFloat(altStr, _state.altitude, 1, 1),
             fmtFloat(speedStr, _state.speedKnots, 1, 1),
             fmtFloat(courseStr, _state.courseTrue, 1, 1),
             fixStatus, _state.numSatellites,
             fmtFloat(hdopStr, _state.hdop, 1, 1),
             _state.hour, _state.minute, _state.second,
             _state.year, _state.month, _state.day,
             (unsigned long)rate);
}

// === Factory Implementation ===

IEmulatedDevice* NMEAGPSDeviceFactory::create(ISerialPort* serial, uint8_t uartIndex) {
    return new NMEAGPSDevice(serial, uartIndex);
}

void NMEAGPSDeviceFactory::destroy(IEmulatedDevice* device) {
    delete device;
}
