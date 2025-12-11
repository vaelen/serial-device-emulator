// Copyright (c) 2025 Andrew C. Young <andrew@vaelen.org>
// SPDX-License-Identifier: MIT

#pragma once

#include <Arduino.h>
#include "ILogger.h"
#include "DeviceOption.h"

// Meter types for console-controlled simulation values
enum class MeterType : uint8_t {
    SMETER = 0,     // S-meter (signal strength)
    POWER,          // TX power
    SWR,            // Standing wave ratio
    ALC,            // Automatic level control
    COMP            // Compression meter
};

// Abstract interface for emulated radio devices
class IEmulatedDevice {
public:
    virtual ~IEmulatedDevice() = default;

    // === Lifecycle ===

    // Initialize the device (configure serial port, reset state)
    // Returns true if successful
    virtual bool begin() = 0;

    // Shutdown the device (close serial port)
    virtual void end() = 0;

    // Process incoming data and update state
    // Called from main loop, must be non-blocking
    virtual void update() = 0;

    // === Identity ===

    // Get device type name (e.g., "yaesu")
    virtual const char* getName() const = 0;

    // Get human-readable description (e.g., "Yaesu FT-991A CAT Emulator")
    virtual const char* getDescription() const = 0;

    // Get unique device instance ID (assigned by DeviceManager)
    virtual uint8_t getDeviceId() const = 0;

    // Set device instance ID (called by DeviceManager)
    virtual void setDeviceId(uint8_t id) = 0;

    // Get UART index this device is using
    virtual uint8_t getUartIndex() const = 0;

    // === Options ===

    // Get number of configurable options
    virtual size_t getOptionCount() const = 0;

    // Get option descriptor by index
    virtual const DeviceOption* getOption(size_t index) const = 0;

    // Find option by name, returns nullptr if not found
    virtual DeviceOption* findOption(const char* name) = 0;

    // Set option value from string, returns true on success
    virtual bool setOption(const char* name, const char* value) = 0;

    // Get option value as string, returns true on success
    virtual bool getOptionValue(const char* name, char* buffer, size_t bufLen) const = 0;

    // === Persistence ===

    // Serialize option values for EEPROM storage
    // Returns number of bytes written, 0 on error
    virtual size_t serializeOptions(uint8_t* buffer, size_t bufLen) const = 0;

    // Deserialize option values from EEPROM storage
    // Returns true on success
    virtual bool deserializeOptions(const uint8_t* buffer, size_t len) = 0;

    // === Meter Simulation ===

    // Set a meter value for simulation (console-controlled)
    virtual bool setMeter(MeterType type, uint8_t value) = 0;

    // Get current meter value
    virtual uint8_t getMeter(MeterType type) const = 0;

    // === Logging ===

    // Set the logger instance for this device
    virtual void setLogger(ILogger* logger) = 0;

    // === Status ===

    // Check if device is currently running
    virtual bool isRunning() const = 0;

    // Get detailed status string for display
    virtual void getStatus(char* buffer, size_t bufLen) const = 0;
};

// Abstract factory for creating device instances
class IDeviceFactory {
public:
    virtual ~IDeviceFactory() = default;

    // Get device type name (e.g., "yaesu")
    virtual const char* getTypeName() const = 0;

    // Get human-readable description
    virtual const char* getDescription() const = 0;

    // Create a new device instance
    // serial: The serial port for this device to use
    // uartIndex: Which UART number (1, 2, etc.)
    // Returns nullptr on failure
    virtual IEmulatedDevice* create(class ISerialPort* serial, uint8_t uartIndex) = 0;

    // Destroy a device instance created by this factory
    virtual void destroy(IEmulatedDevice* device) = 0;
};
