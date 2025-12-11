# CLAUDE.md - Project Context for Claude

## Project Overview

This is a PlatformIO/Arduino project that emulates radio CAT (Computer Aided Transceiver) interfaces. The primary use case is testing and development of software that communicates with amateur radios via their CAT control interface.

## Build Commands

```bash
# Build for STM32 Nucleo L432KC (default)
/home/andrew/.platformio/penv/bin/pio run

# Build for Raspberry Pi Pico
/home/andrew/.platformio/penv/bin/pio run -e pico

# Build all environments
/home/andrew/.platformio/penv/bin/pio run -e nucleo -e pico

# Upload to device
/home/andrew/.platformio/penv/bin/pio run -t upload

# Clean build
/home/andrew/.platformio/penv/bin/pio run -t clean
```

## Project Structure

```
radio-emulator/
├── platformio.ini          # PlatformIO configuration
├── include/                # Public headers (interfaces)
│   ├── platform_config.h   # Platform-specific UART/pin mappings
│   ├── ISerialPort.h       # Abstract serial port interface
│   ├── IEmulatedDevice.h   # Device interface + factory interface
│   ├── ILogger.h           # Logging interface
│   ├── DeviceOption.h      # Configuration option types
│   ├── DeviceManager.h     # Device registry and lifecycle
│   └── ConfigStorage.h     # EEPROM persistence
└── src/
    ├── main.cpp            # Entry point, setup/loop
    ├── console/            # User console implementation
    │   ├── Console.h
    │   └── Console.cpp
    ├── core/               # Core framework implementation
    │   ├── DeviceManager.cpp
    │   ├── HardwareSerialPort.h/.cpp
    │   ├── ConsoleLogger.h/.cpp
    │   └── ConfigStorage.cpp
    └── devices/            # Device implementations
        └── yaesu/
            ├── YaesuDevice.h/.cpp
            ├── YaesuState.h
            └── CATParser.h/.cpp
```

## Architecture

The framework uses a modular architecture with clear separation of concerns:

- **Interfaces in `include/`**: Abstract contracts that define how components interact
- **Core in `src/core/`**: Framework implementation (device management, serial wrappers, logging)
- **Console in `src/console/`**: Command-line interface for user interaction
- **Devices in `src/devices/`**: Specific radio emulator implementations

### Key Interfaces

1. **ISerialPort**: Abstracts hardware serial ports, allowing for testing and portability
2. **IEmulatedDevice**: Contract for emulated devices with lifecycle, options, meter simulation, and serialization
3. **IDeviceFactory**: Factory pattern for creating device instances at runtime
4. **ILogger**: Allows devices to send log messages to the console
5. **ConfigStorage**: Static class for EEPROM persistence of device configuration

### Device Lifecycle

1. Factory registered with DeviceManager at startup
2. ConfigStorage loads saved configuration from EEPROM (if valid)
3. Restored devices are automatically started
4. User can create new devices via console: `create <type> <uart>`
5. DeviceManager allocates UART, creates device instance
6. Configuration auto-saved to EEPROM on create/destroy/set
7. User starts device: `start <id>` - device begins listening on UART
8. Device processes CAT commands in `update()` called from main loop
9. User can stop/destroy device as needed

## Coding Conventions

- **C++11** compatible (Arduino framework constraint)
- Header guards use `#pragma once`
- Class member variables prefixed with `_` (e.g., `_serial`, `_state`)
- Use `const char*` for string literals, avoid `String` class
- Prefer `snprintf` over `sprintf` for buffer safety
- Use `enum class` for type-safe enumerations
- Avoid dynamic allocation where possible; use fixed-size arrays

## Platform-Specific Notes

### STM32 (Nucleo L432KC)

- `USB` is a macro in STM32 headers - avoid using as identifier
- Serial1 requires explicit enabling via build flags:
  ```
  -D ENABLE_HWSERIAL1
  -D PIN_SERIAL1_TX=PA_9
  -D PIN_SERIAL1_RX=PA_10
  ```
- Only 1 additional UART available beyond USB console
- EEPROM uses flash emulation via stm32duino EEPROM library

### Raspberry Pi Pico

- Uses earlephilhower Arduino-Pico core (has EEPROM support)
- Requires `#include <stdarg.h>` for variadic functions
- Only 1 additional UART (Serial1) available beyond USB console
- EEPROM uses flash storage, requires `EEPROM.begin(size)` and `EEPROM.commit()`

## Adding a New Device Type

1. Create directory `src/devices/<name>/`
2. Create state structure (e.g., `<Name>State.h`)
3. Create protocol parser (e.g., `<Name>Parser.h/.cpp`)
4. Implement `IEmulatedDevice` (e.g., `<Name>Device.h/.cpp`)
   - Must implement `serializeOptions()` and `deserializeOptions()` for EEPROM persistence
5. Implement `IDeviceFactory` in the device file
6. Register factory in `main.cpp`:
   ```cpp
   static <Name>DeviceFactory <name>Factory;
   // in setup():
   deviceManager.registerFactory(&<name>Factory);
   ```

## Yaesu CAT Protocol Notes

- Commands are 2 uppercase letters + parameters + `;` terminator
- Read commands: `XX;` - device responds with `XX<value>;`
- Write commands: `XX<value>;` - device updates state, no response
- Frequency is 9 digits in Hz (e.g., `FA014074000;` = 14.074 MHz)
- Mode values: 1=LSB, 2=USB, 3=CW-U, 4=FM, 5=AM, etc.
- Meter values: 0-255 range for most meters

## Common Tasks

### Adding a new CAT command

1. Add handler declaration in `CATParser.h`
2. Implement handler in `CATParser.cpp`
3. Add dispatch in `processCommand()` method
4. Update state in `YaesuState.h` if needed

### Adding a new console command

1. Add handler declaration in `Console.h`
2. Implement handler in `Console.cpp`
3. Add entry to `commands[]` array in `Console.cpp`

### Adding a new device option

1. Increment `YAESU_OPTION_COUNT` (or equivalent)
2. Add option initialization in `initOptions()`
3. Handle option changes in `setOption()` if needed
4. Update `serializeOptions()` to include new option in EEPROM data
5. Update `deserializeOptions()` to restore new option from EEPROM

## Testing

Currently no automated tests. Manual testing procedure:

1. Build and upload to target
2. Connect via serial monitor at 115200 baud
3. Create and start device: `create yaesu 1`, `start 0`
4. Use CAT control software or send commands directly to UART1
5. Verify responses match expected FT-991A behavior

## Known Limitations

- Single UART available for devices on both platforms
- Meter values are console-controlled only (no automatic simulation)
- Limited to CAT commands implemented in parser
- Radio state (frequency, mode, etc.) is not persisted - only device config and options

## EEPROM Storage Format

Configuration uses ~214 bytes in EEPROM:
- Magic number (4 bytes) + version (1 byte) + device count (1 byte)
- Per device: type name (16 bytes) + UART (1 byte) + options (up to 32 bytes)

Auto-save triggers: `create`, `destroy`, `set` commands
Manual commands: `save`, `clear`
