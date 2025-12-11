// Copyright (c) 2025 Andrew C. Young <andrew@vaelen.org>
// SPDX-License-Identifier: MIT

#pragma once

#include <Arduino.h>

// Abstract serial port interface
// Allows devices to use hardware UARTs or software serial implementations
class ISerialPort {
public:
    virtual ~ISerialPort() = default;

    // Initialize the serial port with specified baud rate and configuration
    virtual void begin(uint32_t baud, uint16_t config = SERIAL_8N1) = 0;

    // Shutdown the serial port
    virtual void end() = 0;

    // Returns the number of bytes available to read
    virtual int available() = 0;

    // Read a single byte, returns -1 if no data available
    virtual int read() = 0;

    // Read multiple bytes into buffer, returns number of bytes read
    virtual size_t readBytes(uint8_t* buffer, size_t length) = 0;

    // Write a single byte
    virtual size_t write(uint8_t byte) = 0;

    // Write multiple bytes from buffer
    virtual size_t write(const uint8_t* buffer, size_t size) = 0;

    // Write a null-terminated string
    virtual size_t print(const char* str) = 0;

    // Write a string followed by newline
    virtual size_t println(const char* str) = 0;

    // Wait for outgoing data to be transmitted
    virtual void flush() = 0;

    // Check if port is initialized
    virtual bool isOpen() const = 0;
};
