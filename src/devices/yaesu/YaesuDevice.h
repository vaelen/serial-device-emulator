// Copyright (c) 2025 Andrew C. Young <andrew@vaelen.org>
// SPDX-License-Identifier: MIT

#pragma once

#include "IEmulatedDevice.h"
#include "ISerialPort.h"
#include "YaesuState.h"
#include "CATParser.h"
#include "platform_config.h"

// Number of configurable options
#define YAESU_OPTION_COUNT 2

// Yaesu FT-991A CAT interface emulator
class YaesuDevice : public IEmulatedDevice {
public:
    YaesuDevice(ISerialPort* serial, uint8_t uartIndex);
    ~YaesuDevice() override;

    // === Lifecycle ===
    bool begin() override;
    void end() override;
    void update() override;

    // === Identity ===
    const char* getName() const override { return "yaesu"; }
    const char* getDescription() const override { return "Yaesu FT-991A CAT Emulator"; }
    uint8_t getDeviceId() const override { return _deviceId; }
    void setDeviceId(uint8_t id) override { _deviceId = id; }
    uint8_t getUartIndex() const override { return _uartIndex; }

    // === Options ===
    size_t getOptionCount() const override { return YAESU_OPTION_COUNT; }
    const DeviceOption* getOption(size_t index) const override;
    DeviceOption* findOption(const char* name) override;
    bool setOption(const char* name, const char* value) override;
    bool getOptionValue(const char* name, char* buffer, size_t bufLen) const override;

    // === Persistence ===
    size_t serializeOptions(uint8_t* buffer, size_t bufLen) const override;
    bool deserializeOptions(const uint8_t* buffer, size_t len) override;

    // === Meter Simulation ===
    bool setMeter(MeterType type, uint8_t value) override;
    uint8_t getMeter(MeterType type) const override;

    // === Logging ===
    void setLogger(ILogger* logger) override;

    // === Status ===
    bool isRunning() const override { return _running; }
    void getStatus(char* buffer, size_t bufLen) const override;

private:
    ISerialPort* _serial;
    uint8_t _uartIndex;
    uint8_t _deviceId;
    bool _running;
    ILogger* _logger;

    YaesuState _state;
    CATParser _parser;

    // Options
    DeviceOption _options[YAESU_OPTION_COUNT];

    void initOptions();
    void applyBaudRate();
};

// Factory for creating YaesuDevice instances
class YaesuDeviceFactory : public IDeviceFactory {
public:
    const char* getTypeName() const override { return "yaesu"; }
    const char* getDescription() const override { return "Yaesu FT-991A CAT Emulator"; }
    IEmulatedDevice* create(ISerialPort* serial, uint8_t uartIndex) override;
    void destroy(IEmulatedDevice* device) override;
};
