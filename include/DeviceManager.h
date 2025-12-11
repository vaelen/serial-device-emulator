// Copyright (c) 2025 Andrew C. Young <andrew@vaelen.org>
// SPDX-License-Identifier: MIT

#pragma once

#include <Arduino.h>
#include "platform_config.h"
#include "IEmulatedDevice.h"
#include "ISerialPort.h"
#include "ILogger.h"

// Manages device factories, device instances, and UART allocation
class DeviceManager {
public:
    DeviceManager();
    ~DeviceManager();

    // === Factory Registration ===

    // Register a device factory (e.g., YaesuDeviceFactory)
    bool registerFactory(IDeviceFactory* factory);

    // Get number of registered factories
    size_t getFactoryCount() const { return _factoryCount; }

    // Get factory by index
    const IDeviceFactory* getFactory(size_t index) const;

    // Find factory by type name
    IDeviceFactory* findFactory(const char* typeName);

    // === Device Lifecycle ===

    // Create a new device of the specified type on the given UART
    // Returns device ID on success, 0xFF on failure
    uint8_t createDevice(const char* typeName, uint8_t uartIndex);

    // Create a device and restore its options from serialized data
    // Used during config load from EEPROM
    // Returns device ID on success, 0xFF on failure
    uint8_t createDeviceWithOptions(const char* typeName, uint8_t uartIndex,
                                     const uint8_t* optionData, size_t optionLen);

    // Destroy a device by ID
    bool destroyDevice(uint8_t deviceId);

    // === Device Access ===

    // Get number of active devices
    size_t getDeviceCount() const;

    // Get device by ID (returns nullptr if not found)
    IEmulatedDevice* getDevice(uint8_t deviceId);

    // Get device by UART index (returns nullptr if UART not in use)
    IEmulatedDevice* getDeviceByUart(uint8_t uartIndex);

    // === UART Management ===

    // Check if a UART is available for use
    bool isUartAvailable(uint8_t uartIndex) const;

    // Get serial port wrapper for a UART index
    // Creates wrapper if not already created
    ISerialPort* getSerialForUart(uint8_t uartIndex);

    // === Logger ===

    // Set the logger to use for all devices
    void setLogger(ILogger* logger) { _logger = logger; }

    // === Main Loop ===

    // Call update() on all running devices
    void updateAll();

private:
    // Factory storage
    IDeviceFactory* _factories[MAX_DEVICE_FACTORIES];
    size_t _factoryCount;

    // Device storage (indexed by device ID)
    IEmulatedDevice* _devices[MAX_DEVICES];

    // Track which factory created each device (for proper destruction)
    IDeviceFactory* _deviceFactories[MAX_DEVICES];

    // UART allocation (which device ID is using each UART, 0xFF = free)
    uint8_t _uartAllocation[PLATFORM_MAX_UARTS];

    // Serial port wrappers
    ISerialPort* _serialPorts[PLATFORM_MAX_UARTS];

    // Logger instance
    ILogger* _logger;

    // Next device ID to assign
    uint8_t _nextDeviceId;

    // Find free device slot
    uint8_t findFreeDeviceSlot() const;

    // Initialize serial port for UART
    bool initSerialPort(uint8_t uartIndex);
};
