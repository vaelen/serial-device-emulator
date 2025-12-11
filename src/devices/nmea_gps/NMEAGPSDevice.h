// Copyright (c) 2025 Andrew C. Young <andrew@vaelen.org>
// SPDX-License-Identifier: MIT

#pragma once

#include "IEmulatedDevice.h"
#include "ISerialPort.h"
#include "ILogger.h"
#include "DeviceOption.h"
#include "NMEAGPSState.h"
#include "NMEAGenerator.h"

// Number of configurable options
#define NMEA_GPS_OPTION_COUNT 2

// NMEA GPS device emulator
class NMEAGPSDevice : public IEmulatedDevice {
public:
    NMEAGPSDevice(ISerialPort* serial, uint8_t uartIndex);
    ~NMEAGPSDevice() override;

    // IEmulatedDevice interface
    bool begin() override;
    void end() override;
    void update() override;

    const char* getName() const override { return "nmea-gps"; }
    const char* getDescription() const override { return "NMEA GPS Emulator"; }
    uint8_t getUartIndex() const override { return _uartIndex; }
    bool isRunning() const override { return _running; }

    void setDeviceId(uint8_t id) override { _deviceId = id; }
    uint8_t getDeviceId() const override { return _deviceId; }

    // Options
    size_t getOptionCount() const override { return NMEA_GPS_OPTION_COUNT; }
    const DeviceOption* getOption(size_t index) const override;
    DeviceOption* findOption(const char* name) override;
    bool setOption(const char* name, const char* value) override;
    bool getOptionValue(const char* name, char* buffer, size_t bufLen) const override;

    // Serialization
    size_t serializeOptions(uint8_t* buffer, size_t bufLen) const override;
    bool deserializeOptions(const uint8_t* buffer, size_t len) override;

    // Meters (not applicable for GPS)
    bool setMeter(MeterType type, uint8_t value) override;
    uint8_t getMeter(MeterType type) const override;

    // Logger
    void setLogger(ILogger* logger) override;

    // Status
    void getStatus(char* buffer, size_t bufLen) const override;

    // GPS-specific methods
    void setPosition(double lat, double lon, float alt = 0.0f);
    void setTime(uint8_t hour, uint8_t minute, uint8_t second,
                 uint8_t day = 0, uint8_t month = 0, uint16_t year = 0);
    NMEAGPSState& getState() { return _state; }

private:
    ISerialPort* _serial;
    uint8_t _uartIndex;
    uint8_t _deviceId;
    bool _running;
    ILogger* _logger;

    NMEAGPSState _state;
    NMEAGenerator _generator;
    DeviceOption _options[NMEA_GPS_OPTION_COUNT];

    void initOptions();
    void applyBaudRate();

    // Get update interval in milliseconds based on rate option
    unsigned long getUpdateIntervalMs() const;
};

// Factory for creating NMEA GPS device instances
class NMEAGPSDeviceFactory : public IDeviceFactory {
public:
    const char* getTypeName() const override { return "nmea-gps"; }
    const char* getDescription() const override { return "NMEA GPS Emulator"; }
    DeviceCategory getCategory() const override { return DeviceCategory::GPS; }

    IEmulatedDevice* create(ISerialPort* serial, uint8_t uartIndex) override;
    void destroy(IEmulatedDevice* device) override;
};
