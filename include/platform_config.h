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
    #define PLATFORM_MAX_UARTS 2

    // UART 1: GP0 (TX), GP1 (RX)
    // UART 2: GP8 (TX), GP9 (RX)

    #define HAS_SERIAL1 1
    #define HAS_SERIAL2 1

    // UART pin mappings
    #define UART_1_PINS "TX=GP0, RX=GP1"
    #define UART_2_PINS "TX=GP8, RX=GP9"

#elif defined(ARDUINO_NUCLEO_L432KC) || defined(STM32L4xx)
    // STM32 Nucleo L432KC
    #define PLATFORM_NAME "Nucleo-L432KC"
    #define PLATFORM_MAX_UARTS 1

    // Only Serial1 available on L432KC Arduino framework by default
    // Serial1 uses PA9 (TX), PA10 (RX) - USART1

    #define HAS_SERIAL1 1

    // UART pin mappings
    #define UART_1_PINS "TX=PA9, RX=PA10"

#elif defined(ARDUINO_NUCLEO_G070RB)
    // STM32 Nucleo G070RB (64-pin)
    // Serial2 reserved for ST-Link VCP (console)
    #define PLATFORM_NAME "Nucleo-G070RB"
    #define PLATFORM_MAX_UARTS 4

    #define HAS_SERIAL1 1  // USART1: PA9/PA10
    #define HAS_SERIAL2 0  // Reserved for console
    #define HAS_SERIAL3 1  // USART3: PB10/PB11
    #define HAS_SERIAL4 1  // USART4: PA0/PA1

    // UART pin mappings
    #define UART_1_PINS "TX=PA9, RX=PA10"
    #define UART_3_PINS "TX=PB10, RX=PB11"
    #define UART_4_PINS "TX=PA0, RX=PA1"

#elif defined(ARDUINO_NUCLEO_F091RC)
    // STM32 Nucleo F091RC (64-pin)
    // Serial2 reserved for ST-Link VCP (console)
    #define PLATFORM_NAME "Nucleo-F091RC"
    #define PLATFORM_MAX_UARTS 6

    #define HAS_SERIAL1 1  // USART1: PA9/PA10
    #define HAS_SERIAL2 0  // Reserved for console
    #define HAS_SERIAL3 1  // USART3: PB10/PB11
    #define HAS_SERIAL4 1  // USART4: PA0/PA1
    #define HAS_SERIAL5 1  // USART5: PB3/PB4
    #define HAS_SERIAL6 1  // USART6: PA4/PA5

    // UART pin mappings
    #define UART_1_PINS "TX=PA9, RX=PA10"
    #define UART_3_PINS "TX=PB10, RX=PB11"
    #define UART_4_PINS "TX=PA0, RX=PA1"
    #define UART_5_PINS "TX=PB3, RX=PB4"
    #define UART_6_PINS "TX=PA4, RX=PA5"

#elif defined(ARDUINO_AVR_MEGA2560) || defined(ARDUINO_AVR_MEGA)
    // Arduino Mega 2560
    #define PLATFORM_NAME "Arduino-Mega"
    #define PLATFORM_MAX_UARTS 3

    // Serial1, Serial2, Serial3 available

    #define HAS_SERIAL1 1
    #define HAS_SERIAL2 1
    #define HAS_SERIAL3 1

    // UART pin mappings
    #define UART_1_PINS "TX=18, RX=19"
    #define UART_2_PINS "TX=16, RX=17"
    #define UART_3_PINS "TX=14, RX=15"

#elif defined(ARDUINO_ARCH_ESP32)
    // ESP32
    #define PLATFORM_NAME "ESP32"
    #define PLATFORM_MAX_UARTS 2

    // Serial1 (UART1), Serial2 (UART2) available

    #define HAS_SERIAL1 1
    #define HAS_SERIAL2 1

    // UART pin mappings
    #define UART_1_PINS "TX=17, RX=16"
    #define UART_2_PINS "TX=10, RX=9"

#else
    // Generic fallback
    #define PLATFORM_NAME "Generic"
    #define PLATFORM_MAX_UARTS 8

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

    #ifdef Serial3
        #define HAS_SERIAL3 1
    #else
        #define HAS_SERIAL3 0
    #endif

    #ifdef Serial4
        #define HAS_SERIAL4 1
    #else
        #define HAS_SERIAL4 0
    #endif

    #ifdef Serial5
        #define HAS_SERIAL5 1
    #else
        #define HAS_SERIAL5 0
    #endif

    #ifdef Serial6
        #define HAS_SERIAL6 1
    #else
        #define HAS_SERIAL6 0
    #endif

    #ifdef Serial7
        #define HAS_SERIAL7 1
    #else
        #define HAS_SERIAL7 0
    #endif

    #ifdef Serial8
        #define HAS_SERIAL8 1
    #else
        #define HAS_SERIAL8 0
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
#define MAX_DEVICES 8
#define MAX_DEVICE_FACTORIES 8

// EEPROM configuration
#define EEPROM_SIZE 512  // Bytes to allocate for EEPROM storage

// Default device type aliases for each category
// Used when user specifies category name (e.g., "create radio 1")
#define DEFAULT_RADIO_TYPE "ft-991a"
#define DEFAULT_ROTATOR_TYPE "g-5500"
#define DEFAULT_GPS_TYPE "nmea-gps"

// Get UART pin information string
// Returns pin description for valid UART index, or nullptr if unavailable
inline const char* getUartPins(uint8_t uartIndex) {
    switch (uartIndex) {
#ifdef UART_1_PINS
        case 1: return UART_1_PINS;
#endif
#ifdef UART_2_PINS
        case 2: return UART_2_PINS;
#endif
#ifdef UART_3_PINS
        case 3: return UART_3_PINS;
#endif
#ifdef UART_4_PINS
        case 4: return UART_4_PINS;
#endif
#ifdef UART_5_PINS
        case 5: return UART_5_PINS;
#endif
#ifdef UART_6_PINS
        case 6: return UART_6_PINS;
#endif
#ifdef UART_7_PINS
        case 7: return UART_7_PINS;
#endif
#ifdef UART_8_PINS
        case 8: return UART_8_PINS;
#endif
        default: return nullptr;
    }
}
