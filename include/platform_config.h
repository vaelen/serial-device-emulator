// Copyright (c) 2025 Andrew C. Young <andrew@vaelen.org>
// SPDX-License-Identifier: MIT

#pragma once

#include <Arduino.h>

// Platform-specific UART configuration
// UART 0 is reserved for console (USB Serial)
// UARTs 1+ are available for emulated devices

#if defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_RASPBERRY_PI_PICO)
    // Raspberry Pi Pico
    #define PLATFORM_NAME "Pico"
    #define PLATFORM_MAX_UARTS 1

    // UART 1: GP0 (TX), GP1 (RX)
    #define UART1_TX_PIN 0
    #define UART1_RX_PIN 1

    #define HAS_SERIAL1 1
    #define HAS_SERIAL2 0

#elif defined(ARDUINO_NUCLEO_L432KC) || defined(STM32L4xx)
    // STM32 Nucleo L432KC
    #define PLATFORM_NAME "Nucleo-L432KC"
    #define PLATFORM_MAX_UARTS 1

    // Only Serial1 available on L432KC Arduino framework by default
    // Serial1 uses PA9 (TX), PA10 (RX) - USART1

    #define HAS_SERIAL1 1
    #define HAS_SERIAL2 0

#else
    // Generic fallback
    #define PLATFORM_NAME "Generic"
    #define PLATFORM_MAX_UARTS 1

    #ifdef Serial1
        #define HAS_SERIAL1 1
    #else
        #define HAS_SERIAL1 0
    #endif

    #ifdef Serial2
        #define HAS_SERIAL2 1
    #else
        #define HAS_SERIAL2 0
    #endif
#endif

// Console configuration
#define CONSOLE_BAUD_RATE 115200
#define CONSOLE_PROMPT "> "

// Default device baud rate
#define DEFAULT_DEVICE_BAUD 38400

// Buffer sizes
#define COMMAND_BUFFER_SIZE 128
#define CAT_BUFFER_SIZE 64
#define LOG_BUFFER_SIZE 256

// Maximum devices
#define MAX_DEVICES 4
#define MAX_DEVICE_FACTORIES 8

// EEPROM configuration
#define EEPROM_SIZE 512  // Bytes to allocate for EEPROM storage
