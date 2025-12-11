// Copyright (c) 2025 Andrew C. Young <andrew@vaelen.org>
// SPDX-License-Identifier: MIT

#pragma once

#include "ISerialPort.h"

// Wrapper around Arduino HardwareSerial to implement ISerialPort interface
class HardwareSerialPort : public ISerialPort {
public:
    explicit HardwareSerialPort(HardwareSerial& serial);
    ~HardwareSerialPort() override = default;

    void begin(uint32_t baud, uint16_t config = SERIAL_8N1) override;
    void end() override;
    int available() override;
    int read() override;
    size_t readBytes(uint8_t* buffer, size_t length) override;
    size_t write(uint8_t byte) override;
    size_t write(const uint8_t* buffer, size_t size) override;
    size_t print(const char* str) override;
    size_t println(const char* str) override;
    void flush() override;
    bool isOpen() const override;

private:
    HardwareSerial& _serial;
    bool _isOpen;
};
