// Copyright (c) 2025 Andrew C. Young <andrew@vaelen.org>
// SPDX-License-Identifier: MIT

#pragma once

#include <Arduino.h>
#include "platform_config.h"

// Forward declarations
class DeviceManager;
class IEmulatedDevice;
class ILogger;

// Storage format constants
#define CONFIG_MAGIC 0x52454D55  // "REMU" in little-endian ASCII
#define CONFIG_VERSION 1
#define MAX_TYPE_NAME_LEN 16
#define MAX_OPTION_DATA_LEN 32

// Stored configuration for a single device
struct StoredDeviceConfig {
    uint8_t valid;                          // 0x00 = empty, 0x01 = valid
    char typeName[MAX_TYPE_NAME_LEN];       // Null-terminated device type
    uint8_t uartIndex;                      // UART number (1-based)
    uint8_t optionCount;                    // Number of options stored
    uint8_t optionData[MAX_OPTION_DATA_LEN]; // Serialized option values
};

// Complete stored configuration
struct StoredConfig {
    uint32_t magic;                         // CONFIG_MAGIC
    uint8_t version;                        // CONFIG_VERSION
    uint8_t deviceCount;                    // Number of valid devices
    uint8_t reserved[2];                    // Padding for alignment
    StoredDeviceConfig devices[MAX_DEVICES]; // Device slots
};

// Configuration storage manager
// Handles persistence of device configuration to EEPROM
class ConfigStorage {
public:
    // Initialize EEPROM storage (call in setup before loading)
    static void begin();

    // Set logger for status messages
    static void setLogger(ILogger* logger);

    // Load configuration from EEPROM and restore devices
    // Returns number of devices successfully restored
    static uint8_t load(DeviceManager& mgr);

    // Save current configuration to EEPROM
    // Returns true on success
    static bool save(DeviceManager& mgr);

    // Clear all stored configuration
    static void clear();

    // Check if valid configuration exists in EEPROM
    static bool hasValidConfig();

private:
    static ILogger* _logger;

    // Read stored config from EEPROM
    static bool readConfig(StoredConfig& config);

    // Write stored config to EEPROM
    static bool writeConfig(const StoredConfig& config);

    // Serialize a device to stored format
    static bool serializeDevice(IEmulatedDevice* device, StoredDeviceConfig& config);

    // Restore a device from stored format
    static bool restoreDevice(const StoredDeviceConfig& config, DeviceManager& mgr);
};
