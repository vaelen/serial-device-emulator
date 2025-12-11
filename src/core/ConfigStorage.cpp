// Copyright (c) 2025 Andrew C. Young <andrew@vaelen.org>
// SPDX-License-Identifier: MIT

#include "ConfigStorage.h"
#include "DeviceManager.h"
#include "IEmulatedDevice.h"
#include "ILogger.h"
#include <EEPROM.h>
#include <string.h>

// Static member initialization
ILogger* ConfigStorage::_logger = nullptr;

void ConfigStorage::begin() {
#if defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_RASPBERRY_PI_PICO)
    // Pico requires size parameter
    EEPROM.begin(EEPROM_SIZE);
#else
    // STM32 and other platforms
    EEPROM.begin();
#endif
}

void ConfigStorage::setLogger(ILogger* logger) {
    _logger = logger;
}

bool ConfigStorage::hasValidConfig() {
    uint32_t magic;
    EEPROM.get(0, magic);
    return (magic == CONFIG_MAGIC);
}

bool ConfigStorage::readConfig(StoredConfig& config) {
    EEPROM.get(0, config);

    // Validate magic number
    if (config.magic != CONFIG_MAGIC) {
        return false;
    }

    // Validate version
    if (config.version != CONFIG_VERSION) {
        if (_logger) {
            _logger->logf(LogLevel::WARN, "Config", "Version mismatch: %d vs %d",
                         config.version, CONFIG_VERSION);
        }
        return false;
    }

    return true;
}

bool ConfigStorage::writeConfig(const StoredConfig& config) {
    EEPROM.put(0, config);

#if defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_RASPBERRY_PI_PICO)
    // Pico requires explicit commit
    EEPROM.commit();
#endif

    return true;
}

uint8_t ConfigStorage::load(DeviceManager& mgr) {
    StoredConfig config;

    if (!readConfig(config)) {
        if (_logger) {
            _logger->log(LogLevel::INFO, "Config", "No valid configuration found");
        }
        return 0;
    }

    if (_logger) {
        _logger->logf(LogLevel::INFO, "Config", "Loading %d device(s) from EEPROM",
                     config.deviceCount);
    }

    uint8_t restored = 0;

    for (uint8_t i = 0; i < config.deviceCount && i < MAX_DEVICES; i++) {
        const StoredDeviceConfig& devConfig = config.devices[i];

        if (devConfig.valid != 0x01) {
            continue;
        }

        if (restoreDevice(devConfig, mgr)) {
            restored++;
        }
    }

    if (_logger) {
        _logger->logf(LogLevel::INFO, "Config", "Restored %d device(s)", restored);
    }

    return restored;
}

bool ConfigStorage::save(DeviceManager& mgr) {
    StoredConfig config;
    memset(&config, 0, sizeof(config));

    config.magic = CONFIG_MAGIC;
    config.version = CONFIG_VERSION;
    config.deviceCount = 0;

    // Iterate through all device slots
    for (uint8_t i = 0; i < MAX_DEVICES; i++) {
        IEmulatedDevice* device = mgr.getDevice(i);
        if (device != nullptr) {
            if (serializeDevice(device, config.devices[config.deviceCount])) {
                config.deviceCount++;
            }
        }
    }

    if (!writeConfig(config)) {
        if (_logger) {
            _logger->log(LogLevel::ERROR, "Config", "Failed to write configuration");
        }
        return false;
    }

    if (_logger) {
        _logger->logf(LogLevel::INFO, "Config", "Saved %d device(s) to EEPROM",
                     config.deviceCount);
    }

    return true;
}

void ConfigStorage::clear() {
    StoredConfig config;
    memset(&config, 0, sizeof(config));

    // Write zeroed config (invalid magic)
    EEPROM.put(0, config);

#if defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_RASPBERRY_PI_PICO)
    EEPROM.commit();
#endif

    if (_logger) {
        _logger->log(LogLevel::INFO, "Config", "Configuration cleared");
    }
}

bool ConfigStorage::serializeDevice(IEmulatedDevice* device, StoredDeviceConfig& config) {
    memset(&config, 0, sizeof(config));

    config.valid = 0x01;

    // Copy type name
    strncpy(config.typeName, device->getName(), MAX_TYPE_NAME_LEN - 1);
    config.typeName[MAX_TYPE_NAME_LEN - 1] = '\0';

    // Store UART index
    config.uartIndex = device->getUartIndex();

    // Serialize options
    size_t optionBytes = device->serializeOptions(config.optionData, MAX_OPTION_DATA_LEN);
    config.optionCount = (uint8_t)device->getOptionCount();

    if (_logger) {
        _logger->logf(LogLevel::DEBUG, "Config", "Serialized device '%s' on UART %d (%d option bytes)",
                     config.typeName, config.uartIndex, optionBytes);
    }

    return true;
}

bool ConfigStorage::restoreDevice(const StoredDeviceConfig& config, DeviceManager& mgr) {
    // Validate type name
    if (config.typeName[0] == '\0') {
        if (_logger) {
            _logger->log(LogLevel::WARN, "Config", "Empty device type name");
        }
        return false;
    }

    // Check if UART is available
    if (!mgr.isUartAvailable(config.uartIndex)) {
        if (_logger) {
            _logger->logf(LogLevel::WARN, "Config", "UART %d not available for device '%s'",
                         config.uartIndex, config.typeName);
        }
        return false;
    }

    // Create device with restored options
    uint8_t deviceId = mgr.createDeviceWithOptions(
        config.typeName,
        config.uartIndex,
        config.optionData,
        MAX_OPTION_DATA_LEN
    );

    if (deviceId == 0xFF) {
        if (_logger) {
            _logger->logf(LogLevel::ERROR, "Config", "Failed to create device '%s' on UART %d",
                         config.typeName, config.uartIndex);
        }
        return false;
    }

    if (_logger) {
        _logger->logf(LogLevel::DEBUG, "Config", "Restored device %d ('%s') on UART %d",
                     deviceId, config.typeName, config.uartIndex);
    }

    return true;
}
