// Copyright (c) 2025 Andrew C. Young <andrew@vaelen.org>
// SPDX-License-Identifier: MIT

#include <Arduino.h>

#include "platform_config.h"
#include "DeviceManager.h"
#include "ConfigStorage.h"
#include "core/ConsoleLogger.h"
#include "console/Console.h"
#include "devices/yaesu/YaesuDevice.h"
#include "devices/g5500/G5500Device.h"
#include "devices/nmea_gps/NMEAGPSDevice.h"

// Global instances
static DeviceManager deviceManager;
static ConsoleLogger logger(Serial);
static Console* console = nullptr;

// Device factories
static YaesuDeviceFactory yaesuFactory;
static G5500DeviceFactory g5500Factory;
static NMEAGPSDeviceFactory nmeaGpsFactory;

void setup() {
    // Initialize console serial port
    Serial.begin(CONSOLE_BAUD_RATE);

    // Wait for serial connection (USB)
    while (!Serial) {
        delay(10);
    }

    // Small delay to let terminal connect
    delay(500);

    // Set up logger
    deviceManager.setLogger(&logger);

    // Register device factories
    deviceManager.registerFactory(&yaesuFactory);
    deviceManager.registerFactory(&g5500Factory);
    deviceManager.registerFactory(&nmeaGpsFactory);

    // Initialize configuration storage
    ConfigStorage::begin();
    ConfigStorage::setLogger(&logger);

    // Load saved configuration and auto-start devices
    uint8_t restored = ConfigStorage::load(deviceManager);
    if (restored > 0) {
        // Start all restored devices
        for (uint8_t i = 0; i < MAX_DEVICES; i++) {
            IEmulatedDevice* dev = deviceManager.getDevice(i);
            if (dev != nullptr && !dev->isRunning()) {
                dev->begin();
            }
        }
    }

    // Create console
    console = new Console(Serial, deviceManager, logger);
    console->begin();
}

void loop() {
    // Process console input
    console->update();

    // Update all running devices
    deviceManager.updateAll();
}
