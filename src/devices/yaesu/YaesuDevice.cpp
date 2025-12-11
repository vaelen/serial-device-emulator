// Copyright (c) 2025 Andrew C. Young <andrew@vaelen.org>
// SPDX-License-Identifier: MIT

#include "YaesuDevice.h"
#include <stdio.h>

// Baud rate options
static const char* const baudRateValues[] = {"4800", "9600", "19200", "38400"};
static const uint32_t baudRates[] = {4800, 9600, 19200, 38400};
#define NUM_BAUD_RATES 4
#define DEFAULT_BAUD_INDEX 3  // 38400

YaesuDevice::YaesuDevice(ISerialPort* serial, uint8_t uartIndex)
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

YaesuDevice::~YaesuDevice() {
    if (_running) {
        end();
    }
}

void YaesuDevice::initOptions() {
    // Baud rate option
    _options[0] = makeEnumOption(
        "baud_rate",
        "Serial baud rate",
        baudRateValues,
        NUM_BAUD_RATES,
        DEFAULT_BAUD_INDEX
    );

    // Echo option (log all CAT traffic)
    _options[1] = makeBoolOption(
        "echo",
        "Echo CAT commands to console",
        false
    );
}

bool YaesuDevice::begin() {
    if (_running) {
        return true;
    }

    if (_serial == nullptr) {
        if (_logger) {
            _logger->logf(LogLevel::ERROR, "Yaesu", "No serial port configured");
        }
        return false;
    }

    applyBaudRate();
    _parser.reset();
    _running = true;

    if (_logger) {
        _logger->logf(LogLevel::INFO, "Yaesu", "Started on UART %d at %s baud",
                      _uartIndex,
                      baudRateValues[_options[0].value.enumVal.current]);
    }

    return true;
}

void YaesuDevice::end() {
    if (!_running) {
        return;
    }

    _serial->end();
    _running = false;

    if (_logger) {
        _logger->logf(LogLevel::INFO, "Yaesu", "Stopped device %d", _deviceId);
    }
}

void YaesuDevice::update() {
    if (!_running) {
        return;
    }

    _parser.update();
}

void YaesuDevice::applyBaudRate() {
    uint8_t baudIndex = _options[0].value.enumVal.current;
    uint32_t baud = baudRates[baudIndex];

    if (_serial->isOpen()) {
        _serial->end();
    }

    _serial->begin(baud);

    if (_logger) {
        _logger->logf(LogLevel::DEBUG, "Yaesu", "Baud rate set to %lu", (unsigned long)baud);
    }
}

const DeviceOption* YaesuDevice::getOption(size_t index) const {
    if (index >= YAESU_OPTION_COUNT) {
        return nullptr;
    }
    return &_options[index];
}

DeviceOption* YaesuDevice::findOption(const char* name) {
    for (size_t i = 0; i < YAESU_OPTION_COUNT; i++) {
        if (strcasecmp(_options[i].name, name) == 0) {
            return &_options[i];
        }
    }
    return nullptr;
}

bool YaesuDevice::setOption(const char* name, const char* value) {
    DeviceOption* opt = findOption(name);
    if (opt == nullptr) {
        return false;
    }

    bool success = parseOptionValue(*opt, value);

    // Apply baud rate change if device is running
    if (success && strcmp(name, "baud_rate") == 0 && _running) {
        applyBaudRate();
    }

    return success;
}

bool YaesuDevice::getOptionValue(const char* name, char* buffer, size_t bufLen) const {
    for (size_t i = 0; i < YAESU_OPTION_COUNT; i++) {
        if (strcasecmp(_options[i].name, name) == 0) {
            formatOptionValue(_options[i], buffer, bufLen);
            return true;
        }
    }
    return false;
}

bool YaesuDevice::setMeter(MeterType type, uint8_t value) {
    switch (type) {
        case MeterType::SMETER:
            _state.smeter = value;
            break;
        case MeterType::POWER:
            _state.powerMeter = value;
            break;
        case MeterType::SWR:
            _state.swrMeter = value;
            break;
        case MeterType::ALC:
            _state.alcMeter = value;
            break;
        case MeterType::COMPRESSION:
            _state.compMeter = value;
            break;
        default:
            return false;
    }

    if (_logger) {
        _logger->logf(LogLevel::DEBUG, "Yaesu", "Meter %d set to %d", (int)type, value);
    }

    return true;
}

uint8_t YaesuDevice::getMeter(MeterType type) const {
    switch (type) {
        case MeterType::SMETER: return _state.smeter;
        case MeterType::POWER:  return _state.powerMeter;
        case MeterType::SWR:    return _state.swrMeter;
        case MeterType::ALC:    return _state.alcMeter;
        case MeterType::COMPRESSION:   return _state.compMeter;
        default: return 0;
    }
}

void YaesuDevice::setLogger(ILogger* logger) {
    _logger = logger;
    _parser.setLogger(logger);
}

void YaesuDevice::getStatus(char* buffer, size_t bufLen) const {
    const char* modeNames[] = {
        "???", "LSB", "USB", "CW-U", "FM", "AM", "RTTY-L", "CW-L",
        "DATA-L", "RTTY-U", "DATA-FM", "FM-N", "DATA-U", "AM-N", "C4FM"
    };

    uint8_t modeIdx = (uint8_t)_state.getCurrentMode();
    if (modeIdx > 14) modeIdx = 0;

    snprintf(buffer, bufLen,
             "  VFO-A: %lu Hz (%s)\n"
             "  VFO-B: %lu Hz\n"
             "  Active VFO: %c\n"
             "  PTT: %s\n"
             "  S-Meter: %d\n"
             "  RIT: %s (%+d Hz)\n"
             "  XIT: %s (%+d Hz)",
             (unsigned long)_state.freqVfoA,
             modeNames[modeIdx],
             (unsigned long)_state.freqVfoB,
             (_state.currentVfo == YaesuVFO::VFO_A) ? 'A' : 'B',
             _state.ptt ? "ON" : "OFF",
             _state.smeter,
             _state.ritOn ? "ON" : "OFF",
             _state.ritOffset,
             _state.xitOn ? "ON" : "OFF",
             _state.xitOffset);
}

// === Persistence ===

size_t YaesuDevice::serializeOptions(uint8_t* buffer, size_t bufLen) const {
    // Format: [baud_rate_index (1 byte)] [echo (1 byte)]
    if (bufLen < 2) {
        return 0;
    }

    buffer[0] = _options[0].value.enumVal.current;  // baud_rate
    buffer[1] = _options[1].value.boolVal ? 1 : 0;  // echo

    return 2;
}

bool YaesuDevice::deserializeOptions(const uint8_t* buffer, size_t len) {
    if (len < 2) {
        return false;
    }

    // Validate baud rate index
    uint8_t baudIndex = buffer[0];
    if (baudIndex >= NUM_BAUD_RATES) {
        baudIndex = DEFAULT_BAUD_INDEX;
    }
    _options[0].value.enumVal.current = baudIndex;

    // Restore echo setting
    _options[1].value.boolVal = (buffer[1] != 0);

    return true;
}

// === Factory Implementation ===

IEmulatedDevice* YaesuDeviceFactory::create(ISerialPort* serial, uint8_t uartIndex) {
    return new YaesuDevice(serial, uartIndex);
}

void YaesuDeviceFactory::destroy(IEmulatedDevice* device) {
    delete device;
}
