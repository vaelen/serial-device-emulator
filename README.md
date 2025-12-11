# Serial Device Emulator

A PlatformIO/Arduino framework for emulating radio CAT (Computer Aided Transceiver), antenna rotator controllers, GPS receivers, and other serial interfaces. The emulator supports multiple concurrent emulated devices, each controlling its own UART, with an interactive command-line console on the primary Serial port.

## Supported Platforms

- **STM32 Nucleo L432KC** - 1 emulated device UART available
- **Raspberry Pi Pico** - 1 emulated device UART available

## Supported Devices

- **Yaesu FT-991A** - Full CAT protocol emulation with read/write support

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        User Console                              │
│                    (Primary Serial/USB)                          │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │ CommandParser → Console → DeviceManager                     │ │
│  └─────────────────────────────────────────────────────────────┘ │
└────────────────────────────┬────────────────────────────────────┘
                             │ Logger + Options Interface
                             ▼
                    ┌───────────────┐
                    │ EmulatedDevice│
                    │   (Yaesu)     │
                    │   Serial1     │
                    └───────────────┘
```

The framework is designed around several key interfaces:

- **ISerialPort** - Abstract serial port interface for hardware abstraction
- **IEmulatedDevice** - Contract for emulated radio devices
- **IDeviceFactory** - Factory pattern for dynamic device creation
- **ILogger** - Logging interface for device-to-console communication
- **DeviceOption** - Configuration option system for runtime settings
- **ConfigStorage** - EEPROM persistence for device configuration

## Building

### Prerequisites

- [PlatformIO](https://platformio.org/) (CLI or IDE)

### Build Commands

```bash
# Build for default target (Nucleo L432KC)
pio run

# Build for specific target
pio run -e nucleo
pio run -e pico

# Upload to connected device
pio run -t upload

# Open serial monitor
pio device monitor
```

## Usage

1. Connect to the device via serial monitor at **115200 baud**
2. You'll see the welcome message and prompt
3. Create and start a device:

```
> create yaesu 1
Created device 0
> start 0
Started device 0
```

4. The emulated radio is now listening on Serial1 (UART1)
5. Connect your CAT control software to the device's UART1 pins

### Console Commands

| Command | Description |
|---------|-------------|
| `help [cmd]` | Show help for all or specific command |
| `types` | List available device types |
| `devices` | List active device instances |
| `create <type> <uart>` | Create device on specified UART |
| `destroy <id>` | Destroy device by ID |
| `start <id>` | Start a device |
| `stop <id>` | Stop a device |
| `status [id]` | Show device status |
| `options <id>` | List device configuration options |
| `set <id> <opt> <val>` | Set device option |
| `get <id> <opt>` | Get device option value |
| `log <level>` | Set log level (debug/info/warn/error) |
| `smeter <id> <val>` | Set S-meter value (0-255) |
| `power <id> <val>` | Set power meter value |
| `swr <id> <val>` | Set SWR meter value |
| `save` | Save configuration to EEPROM |
| `clear` | Clear stored configuration |

### Example Session

```
=================================
  Radio Emulator Console
  Platform: Nucleo-L432KC
  Available UARTs: 1
=================================
Type 'help' for available commands.

> types
Available device types:
  yaesu        - Yaesu FT-991A CAT Emulator

> create yaesu 1
[INF] [DevMgr] Created device 0 (yaesu) on UART 1
Created device 0

> options 0
Options for device 0:
  baud_rate        = 38400        (Serial baud rate)
  echo             = false        (Echo CAT commands to console)

> set 0 baud_rate 9600
Set baud_rate = 9600

> start 0
[INF] [Yaesu] Started on UART 1 at 9600 baud
Started device 0

> status 0
Device 0 (yaesu):
  Description: Yaesu FT-991A CAT Emulator
  UART: 1
  Status: running
  VFO-A: 14074000 Hz (USB)
  VFO-B: 7074000 Hz
  Active VFO: A
  PTT: OFF
  S-Meter: 0
  RIT: OFF (+0 Hz)
  XIT: OFF (+0 Hz)

> smeter 0 9
S-meter set to 9

> log debug
Log level set to: debug
```

### Configuration Persistence

Device configuration is automatically saved to EEPROM and restored on boot:

- **Auto-save**: Configuration is saved automatically when you `create`, `destroy`, or `set` device options
- **Auto-restore**: On boot, saved devices are automatically recreated and started
- **Manual save**: Use `save` command to explicitly save configuration
- **Clear config**: Use `clear` command to wipe stored configuration

What is saved:
- Device type (e.g., "yaesu")
- UART assignment
- Device options (baud rate, echo setting, etc.)

What is NOT saved:
- Radio simulation state (frequency, mode, PTT, etc.)
- Meter values

## Yaesu CAT Protocol

The emulator implements the Yaesu "New CAT" protocol used by the FT-991A and similar radios.

### Protocol Format

- Commands: 2 alphabetical characters + parameters + `;` terminator
- Case insensitive
- Supported baud rates: 4800, 9600, 19200, 38400

### Implemented Commands

| Command | Description | Format |
|---------|-------------|--------|
| FA | VFO-A frequency | `FA;` (read) / `FA#########;` (set, 9 digits Hz) |
| FB | VFO-B frequency | `FB;` (read) / `FB#########;` (set) |
| IF | Information | `IF;` (read only, returns status string) |
| ID | Radio ID | `ID;` → `ID0670;` (FT-991A) |
| MD | Mode | `MD0;` (read) / `MD0#;` (set, 1=LSB, 2=USB, etc.) |
| PS | Power status | `PS;` / `PS#;` (0=off, 1=on) |
| SM | S-Meter | `SM0;` → `SM0###;` (0-255) |
| TX | PTT | `TX;` (read) / `TX#;` (0=RX, 1=TX) |
| RX | Receive | `RX;` (sets PTT off) |
| VS | VFO Select | `VS;` / `VS#;` (0=A, 1=B) |
| RI | RIT on/off | `RI;` / `RI#;` |
| XT | XIT on/off | `XT;` / `XT#;` |
| RD | RIT down | `RD;` / `RD####;` |
| RU | RIT up | `RU;` / `RU####;` |
| AG | AF gain | `AG0;` / `AG0###;` (0-255) |
| RG | RF gain | `RG0;` / `RG0###;` (0-255) |
| SQ | Squelch | `SQ0;` / `SQ0###;` (0-100) |
| RM | Read meter | `RM#;` (1=S, 2=Power, 3=SWR, 4=ALC, 5=Comp) |

## Hardware Connections

### STM32 Nucleo L432KC

| Function | Pin |
|----------|-----|
| Console TX | USB |
| Console RX | USB |
| Device 1 TX | PA9 |
| Device 1 RX | PA10 |

### Raspberry Pi Pico

| Function | Pin |
|----------|-----|
| Console TX | USB |
| Console RX | USB |
| Device 1 TX | GP0 |
| Device 1 RX | GP1 |

## Adding New Device Types

To add support for a new radio:

1. Create a new directory under `src/devices/`
2. Implement the device state structure
3. Implement the protocol parser
4. Create a class implementing `IEmulatedDevice`
5. Create a factory class implementing `IDeviceFactory`
6. Register the factory in `main.cpp`

See `src/devices/yaesu/` for a complete example.

## References

- [Yaesu FT-991A CAT Manual (Official)](https://www.yaesu.com/Files/4CB893D7-1018-01AF-FA97E9E9AD48B50C/FT-991A_CAT_OM_ENG_1711-D.pdf)
- [Hamlib newcat.c](https://github.com/Derecho/hamlib/blob/master/yaesu/newcat.c) - Reference implementation

## License

MIT License
