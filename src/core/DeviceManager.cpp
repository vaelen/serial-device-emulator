// Copyright (c) 2025 Andrew C. Young <andrew@vaelen.org>
// SPDX-License-Identifier: MIT

#include "DeviceManager.h"
#include "HardwareSerialPort.h"

// Invalid device ID marker
#define INVALID_ID 0xFF

DeviceManager::DeviceManager()
    : _factoryCount(0)
    , _logger(nullptr)
    , _nextDeviceId(0)
{
    // Initialize arrays
    for (size_t i = 0; i < MAX_DEVICE_FACTORIES; i++) {
        _factories[i] = nullptr;
    }
    for (size_t i = 0; i < MAX_DEVICES; i++) {
        _devices[i] = nullptr;
        _deviceFactories[i] = nullptr;
    }
    for (size_t i = 0; i < PLATFORM_MAX_UARTS; i++) {
        _uartAllocation[i] = INVALID_ID;
        _serialPorts[i] = nullptr;
    }
}

DeviceManager::~DeviceManager() {
    // Destroy all devices
    for (size_t i = 0; i < MAX_DEVICES; i++) {
        if (_devices[i] != nullptr) {
            destroyDevice(i);
        }
    }

    // Clean up serial port wrappers
    for (size_t i = 0; i < PLATFORM_MAX_UARTS; i++) {
        delete _serialPorts[i];
        _serialPorts[i] = nullptr;
    }
}

bool DeviceManager::registerFactory(IDeviceFactory* factory) {
    if (factory == nullptr || _factoryCount >= MAX_DEVICE_FACTORIES) {
        return false;
    }

    // Check for duplicate type name
    for (size_t i = 0; i < _factoryCount; i++) {
        if (strcmp(_factories[i]->getTypeName(), factory->getTypeName()) == 0) {
            return false;  // Already registered
        }
    }

    _factories[_factoryCount++] = factory;
    return true;
}

const IDeviceFactory* DeviceManager::getFactory(size_t index) const {
    if (index >= _factoryCount) {
        return nullptr;
    }
    return _factories[index];
}

IDeviceFactory* DeviceManager::findFactory(const char* typeName) {
    for (size_t i = 0; i < _factoryCount; i++) {
        if (strcasecmp(_factories[i]->getTypeName(), typeName) == 0) {
            return _factories[i];
        }
    }
    return nullptr;
}

const char* DeviceManager::resolveTypeName(const char* typeOrCategory) {
    if (typeOrCategory == nullptr) {
        return typeOrCategory;
    }

    // Check if input is a category name and map to default type
    if (strcasecmp(typeOrCategory, "radio") == 0) {
        if (strlen(DEFAULT_RADIO_TYPE) > 0) {
            return DEFAULT_RADIO_TYPE;
        }
    } else if (strcasecmp(typeOrCategory, "rotator") == 0) {
        if (strlen(DEFAULT_ROTATOR_TYPE) > 0) {
            return DEFAULT_ROTATOR_TYPE;
        }
    } else if (strcasecmp(typeOrCategory, "gps") == 0) {
        if (strlen(DEFAULT_GPS_TYPE) > 0) {
            return DEFAULT_GPS_TYPE;
        }
    }

    // Return unchanged if not a category or no default defined
    return typeOrCategory;
}

uint8_t DeviceManager::createDevice(const char* typeName, uint8_t uartIndex) {
    // Resolve category aliases to actual type names
    const char* resolvedType = resolveTypeName(typeName);

    // Validate UART index
    if (uartIndex == 0 || uartIndex > PLATFORM_MAX_UARTS) {
        if (_logger) {
            _logger->logf(LogLevel::ERROR, "DevMgr", "Invalid UART index: %d", uartIndex);
        }
        return INVALID_ID;
    }

    // Check UART availability
    if (!isUartAvailable(uartIndex)) {
        if (_logger) {
            _logger->logf(LogLevel::ERROR, "DevMgr", "UART %d is already in use", uartIndex);
        }
        return INVALID_ID;
    }

    // Find factory
    IDeviceFactory* factory = findFactory(resolvedType);
    if (factory == nullptr) {
        if (_logger) {
            _logger->logf(LogLevel::ERROR, "DevMgr", "Unknown device type: %s", typeName);
        }
        return INVALID_ID;
    }

    // Find free device slot
    uint8_t slot = findFreeDeviceSlot();
    if (slot == INVALID_ID) {
        if (_logger) {
            _logger->log(LogLevel::ERROR, "DevMgr", "No free device slots");
        }
        return INVALID_ID;
    }

    // Get or create serial port
    ISerialPort* serial = getSerialForUart(uartIndex);
    if (serial == nullptr) {
        if (_logger) {
            _logger->logf(LogLevel::ERROR, "DevMgr", "Failed to get serial for UART %d", uartIndex);
        }
        return INVALID_ID;
    }

    // Create device
    IEmulatedDevice* device = factory->create(serial, uartIndex);
    if (device == nullptr) {
        if (_logger) {
            _logger->log(LogLevel::ERROR, "DevMgr", "Factory failed to create device");
        }
        return INVALID_ID;
    }

    // Assign ID and logger
    uint8_t deviceId = slot;
    device->setDeviceId(deviceId);
    device->setLogger(_logger);

    // Store device
    _devices[slot] = device;
    _deviceFactories[slot] = factory;
    _uartAllocation[uartIndex - 1] = deviceId;  // uartIndex is 1-based, array is 0-based

    if (_logger) {
        _logger->logf(LogLevel::INFO, "DevMgr", "Created device %d (%s) on UART %d",
                      deviceId, resolvedType, uartIndex);
    }

    return deviceId;
}

uint8_t DeviceManager::createDeviceWithOptions(const char* typeName, uint8_t uartIndex,
                                                const uint8_t* optionData, size_t optionLen) {
    // Create the device first
    uint8_t deviceId = createDevice(typeName, uartIndex);
    if (deviceId == INVALID_ID) {
        return INVALID_ID;
    }

    // Restore options if data provided
    if (optionData != nullptr && optionLen > 0) {
        IEmulatedDevice* device = _devices[deviceId];
        if (!device->deserializeOptions(optionData, optionLen)) {
            if (_logger) {
                _logger->logf(LogLevel::WARN, "DevMgr", "Failed to restore options for device %d", deviceId);
            }
            // Continue anyway - device will use defaults
        }
    }

    return deviceId;
}

bool DeviceManager::destroyDevice(uint8_t deviceId) {
    if (deviceId >= MAX_DEVICES || _devices[deviceId] == nullptr) {
        return false;
    }

    IEmulatedDevice* device = _devices[deviceId];
    IDeviceFactory* factory = _deviceFactories[deviceId];

    // Stop device if running
    if (device->isRunning()) {
        device->end();
    }

    // Free UART allocation
    uint8_t uartIndex = device->getUartIndex();
    if (uartIndex > 0 && uartIndex <= PLATFORM_MAX_UARTS) {
        _uartAllocation[uartIndex - 1] = INVALID_ID;
    }

    // Destroy device through its factory
    if (factory != nullptr) {
        factory->destroy(device);
    }

    _devices[deviceId] = nullptr;
    _deviceFactories[deviceId] = nullptr;

    if (_logger) {
        _logger->logf(LogLevel::INFO, "DevMgr", "Destroyed device %d", deviceId);
    }

    return true;
}

size_t DeviceManager::getDeviceCount() const {
    size_t count = 0;
    for (size_t i = 0; i < MAX_DEVICES; i++) {
        if (_devices[i] != nullptr) {
            count++;
        }
    }
    return count;
}

IEmulatedDevice* DeviceManager::getDevice(uint8_t deviceId) {
    if (deviceId >= MAX_DEVICES) {
        return nullptr;
    }
    return _devices[deviceId];
}

IEmulatedDevice* DeviceManager::getDeviceByUart(uint8_t uartIndex) {
    if (uartIndex == 0 || uartIndex > PLATFORM_MAX_UARTS) {
        return nullptr;
    }
    uint8_t deviceId = _uartAllocation[uartIndex - 1];
    if (deviceId == INVALID_ID) {
        return nullptr;
    }
    return _devices[deviceId];
}

bool DeviceManager::isUartAvailable(uint8_t uartIndex) const {
    if (uartIndex == 0 || uartIndex > PLATFORM_MAX_UARTS) {
        return false;
    }

    // Check platform support
#if !HAS_SERIAL1
    if (uartIndex == 1) return false;
#endif
#if !HAS_SERIAL2
    if (uartIndex == 2) return false;
#endif

    // Check if already allocated
    return _uartAllocation[uartIndex - 1] == INVALID_ID;
}

ISerialPort* DeviceManager::getSerialForUart(uint8_t uartIndex) {
    if (uartIndex == 0 || uartIndex > PLATFORM_MAX_UARTS) {
        return nullptr;
    }

    // Return existing wrapper if already created
    if (_serialPorts[uartIndex - 1] != nullptr) {
        return _serialPorts[uartIndex - 1];
    }

    // Create new wrapper
    HardwareSerial* hwSerial = nullptr;

    switch (uartIndex) {
#if HAS_SERIAL1
        case 1:
            hwSerial = &Serial1;
            break;
#endif
#if HAS_SERIAL2
        case 2:
            hwSerial = &Serial2;
            break;
#endif
        default:
            return nullptr;
    }

    if (hwSerial == nullptr) {
        return nullptr;
    }

    _serialPorts[uartIndex - 1] = new HardwareSerialPort(*hwSerial);
    return _serialPorts[uartIndex - 1];
}

void DeviceManager::updateAll() {
    for (size_t i = 0; i < MAX_DEVICES; i++) {
        if (_devices[i] != nullptr && _devices[i]->isRunning()) {
            _devices[i]->update();
        }
    }
}

uint8_t DeviceManager::findFreeDeviceSlot() const {
    for (size_t i = 0; i < MAX_DEVICES; i++) {
        if (_devices[i] == nullptr) {
            return i;
        }
    }
    return INVALID_ID;
}
