// Copyright (c) 2025 Andrew C. Young <andrew@vaelen.org>
// SPDX-License-Identifier: MIT

#include "HardwareSerialPort.h"

HardwareSerialPort::HardwareSerialPort(HardwareSerial& serial)
    : _serial(serial)
    , _isOpen(false)
{
}

void HardwareSerialPort::begin(uint32_t baud, uint16_t config) {
    _serial.begin(baud, config);
    _isOpen = true;
}

void HardwareSerialPort::end() {
    _serial.end();
    _isOpen = false;
}

int HardwareSerialPort::available() {
    return _serial.available();
}

int HardwareSerialPort::read() {
    return _serial.read();
}

size_t HardwareSerialPort::readBytes(uint8_t* buffer, size_t length) {
    return _serial.readBytes(buffer, length);
}

size_t HardwareSerialPort::write(uint8_t byte) {
    return _serial.write(byte);
}

size_t HardwareSerialPort::write(const uint8_t* buffer, size_t size) {
    return _serial.write(buffer, size);
}

size_t HardwareSerialPort::print(const char* str) {
    return _serial.print(str);
}

size_t HardwareSerialPort::println(const char* str) {
    return _serial.println(str);
}

void HardwareSerialPort::flush() {
    _serial.flush();
}

bool HardwareSerialPort::isOpen() const {
    return _isOpen;
}
