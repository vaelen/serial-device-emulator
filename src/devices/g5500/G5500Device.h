// Copyright (c) 2025 Andrew C. Young <andrew@vaelen.org>
// SPDX-License-Identifier: MIT

#pragma once

#include "IEmulatedDevice.h"
#include "ISerialPort.h"
#include "G5500State.h"
#include "GS232Parser.h"
#include "platform_config.h"

// Number of configurable options
#define G5500_OPTION_COUNT 3

// Yaesu G-5500 Az/El rotator emulator with GS-232 protocol
class G5500Device : public IEmulatedDevice {
public:
    G5500Device(ISerialPort* serial, uint8_t uartIndex);
    ~G5500Device() override;

    // === Lifecycle ===
    bool begin() override;
    void end() override;
    void update() override;

    // === Identity ===
    const char* getName() const override { return "g-5500"; }
    const char* getDescription() const override { return "Yaesu G-5500 Rotator (GS-232)"; }
    uint8_t getDeviceId() const override { return _deviceId; }
    void setDeviceId(uint8_t id) override { _deviceId = id; }
    uint8_t getUartIndex() const override { return _uartIndex; }

    // === Options ===
    size_t getOptionCount() const override { return G5500_OPTION_COUNT; }
    const DeviceOption* getOption(size_t index) const override;
    DeviceOption* findOption(const char* name) override;
    bool setOption(const char* name, const char* value) override;
    bool getOptionValue(const char* name, char* buffer, size_t bufLen) const override;

    // === Persistence ===
    size_t serializeOptions(uint8_t* buffer, size_t bufLen) const override;
    bool deserializeOptions(const uint8_t* buffer, size_t len) override;

    // === Meter Simulation (not applicable for rotators) ===
    bool setMeter(MeterType type, uint8_t value) override;
    uint8_t getMeter(MeterType type) const override;

    // === Logging ===
    void setLogger(ILogger* logger) override;

    // === Status ===
    bool isRunning() const override { return _running; }
    void getStatus(char* buffer, size_t bufLen) const override;

    // === Rotator-specific accessors ===
    G5500State& getState() { return _state; }
    const G5500State& getState() const { return _state; }
    float getAzSpeed() const;
    float getElSpeed() const;

private:
    ISerialPort* _serial;
    uint8_t _uartIndex;
    uint8_t _deviceId;
    bool _running;
    ILogger* _logger;

    G5500State _state;
    GS232Parser _parser;

    // Options
    DeviceOption _options[G5500_OPTION_COUNT];

    void initOptions();
    void applyBaudRate();
    void simulateRotation();
};

// Factory for creating G5500Device instances
class G5500DeviceFactory : public IDeviceFactory {
public:
    const char* getTypeName() const override { return "g-5500"; }
    const char* getDescription() const override { return "Yaesu G-5500 Rotator (GS-232)"; }
    DeviceCategory getCategory() const override { return DeviceCategory::ROTATOR; }
    IEmulatedDevice* create(ISerialPort* serial, uint8_t uartIndex) override;
    void destroy(IEmulatedDevice* device) override;
};
