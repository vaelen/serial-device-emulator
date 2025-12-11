# Serial Device Emulator

A PlatformIO/Arduino framework for emulating radio CAT (Computer Aided Transceiver), antenna rotator controllers, GPS receivers, and other serial interfaces. The emulator supports multiple concurrent emulated devices, each controlling its own UART, with an interactive command-line console on the primary Serial port.

## Supported Platforms

| Platform | Environment | Device UARTs | Notes |
|----------|-------------|--------------|-------|
| STM32 Nucleo L432KC | `nucleo-32-l432kc` | 1 | Default build target |
| STM32 Nucleo G070RB | `nucleo-64-g070rb` | 3 | Serial2 reserved for console |
| STM32 Nucleo F091RC | `nucleo-64-f091rc` | 5 | Serial2 reserved for console |
| Raspberry Pi Pico | `pico` | 2 | Uses earlephilhower core |
| Arduino Mega 2560 | `arduino-mega2560` | 3 | |
| ESP32 | `esp32dev` | 2 | |

## Supported Devices

### Radio
- **Yaesu FT-991A** (`ft-991a`) - Full CAT protocol emulation with read/write support

### Rotator
- **Yaesu G-5500** (`g-5500`) - Az/El rotator with GS-232 protocol, realistic rotation simulation

### GPS
- **NMEA GPS** (`nmea-gps`) - Standard NMEA 0183 GPS emulator with configurable output rate

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
pio run -e nucleo-32-l432kc    # STM32 Nucleo L432KC (default)
pio run -e nucleo-64-g070rb    # STM32 Nucleo G070RB
pio run -e nucleo-64-f091rc    # STM32 Nucleo F091RC
pio run -e pico                # Raspberry Pi Pico
pio run -e arduino-mega2560    # Arduino Mega 2560
pio run -e esp32dev            # ESP32

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
> create radio 1
Created device 0
> start 0
Started device 0
```

You can use category aliases (`radio`, `rotator`, `gps`) or specific device names (`ft-991a`, `g-5500`).

4. The emulated device is now listening on Serial1 (UART1)
5. Connect your control software to the device's UART1 pins

### Console Commands

| Command | Description |
|---------|-------------|
| `help [cmd]` | Show help for all or specific command |
| `types` | List available device types |
| `uarts` | List available UARTs with pin assignments |
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
| `gps <id> <lat> <lon> [alt]` | Set GPS position (decimal degrees) |
| `save` | Save configuration to EEPROM |
| `clear` | Clear stored configuration |

### Example Session

```
=================================
  Radio Emulator Console
  Platform: Nucleo-L432KC
  Available UARTs: 1
  UARTs: 1(TX=PA9, RX=PA10)
=================================
Type 'help' for available commands.

> types
Available device types:

  Radio:
    ft-991a      - Yaesu FT-991A CAT Emulator

  Rotator:
    g-5500       - Yaesu G-5500 Rotator (GS-232)

  GPS:
    nmea-gps     - NMEA GPS Emulator

> create radio 1
[INF] [DevMgr] Created device 0 (ft-991a) on UART 1
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
Device 0 (ft-991a):
  Description: Yaesu FT-991A CAT Emulator
  UART: 1 (TX=PA9, RX=PA10)
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

## Yaesu FT-991A CAT Protocol

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

## Yaesu G-5500 GS-232 Protocol

The rotator emulator implements the GS-232A/B protocol used by Yaesu rotator controllers.

### Protocol Format

- Commands: Single letter + optional parameters + CR terminator
- Case insensitive
- Supported baud rates: 1200, 4800, 9600

### Rotation Simulation

The emulator simulates realistic rotation:
- Configurable azimuth speed (1-10 deg/sec, default 2)
- Configurable elevation speed (1-10 deg/sec, default 1)
- Azimuth range: 0-450 degrees (allows north overlap)
- Elevation range: 0-180 degrees

### Implemented Commands

| Command | Description | Response |
|---------|-------------|----------|
| R | Rotate CW (azimuth increase) | - |
| L | Rotate CCW (azimuth decrease) | - |
| A | Stop azimuth rotation | - |
| U | Rotate up (elevation increase) | - |
| D | Rotate down (elevation decrease) | - |
| E | Stop elevation rotation | - |
| S | Full stop (all rotation) | - |
| C | Read azimuth | `+0###` |
| C2 | Read azimuth and elevation | `+0### +0###` |
| B | Read elevation | `+0###` |
| M### | Move to azimuth | - |
| W### ### | Move to azimuth and elevation | - |

### G-5500 Device Options

| Option | Values | Default | Description |
|--------|--------|---------|-------------|
| baud_rate | 1200, 4800, 9600 | 9600 | Serial baud rate |
| az_speed | 1-10 | 2 | Azimuth rotation speed (deg/sec) |
| el_speed | 1-10 | 1 | Elevation rotation speed (deg/sec) |

## NMEA GPS Emulator

The GPS emulator outputs standard NMEA 0183 sentences continuously at a configurable rate.

### Output Sentences

The emulator outputs the following NMEA sentences each update cycle:

| Sentence | Description |
|----------|-------------|
| GGA | GPS Fix Data (position, altitude, satellites, HDOP) |
| RMC | Recommended Minimum (position, speed, course, date/time) |
| GSA | DOP and Active Satellites |
| GSV | Satellites in View (PRN, elevation, azimuth, SNR) |
| VTG | Velocity Made Good (course and speed) |

### Simulated Data

- **Position**: Default San Francisco (37.7749, -122.4194), configurable via `gps` command
- **Time**: Simulated UTC time, advances each second
- **Satellites**: 8 simulated satellites with realistic PRN, elevation, azimuth, and SNR values
- **Fix**: Reports valid GPS fix with 1.0 HDOP by default

### NMEA GPS Device Options

| Option | Values | Default | Description |
|--------|--------|---------|-------------|
| baud_rate | 4800, 9600, 19200, 38400 | 9600 | Serial baud rate |
| update_rate | 1, 5, 10 | 1 | Output rate in Hz |

### Setting GPS Position

Use the `gps` command to set the emulated position:

```
> gps 0 40.7128 -74.0060        # New York (lat, lon)
GPS position set to 40.712800, -74.006000

> gps 0 51.5074 -0.1278 15.5    # London with altitude
GPS position set to 51.507400, -0.127800, 15.5m
```

## Hardware Connections

### STM32 Nucleo L432KC

| UART | TX Pin | RX Pin | Notes |
|------|--------|--------|-------|
| Console | USB | USB | Primary serial |
| UART 1 | PA9 | PA10 | |

### STM32 Nucleo G070RB

| UART | TX Pin | RX Pin | Notes |
|------|--------|--------|-------|
| Console | ST-Link VCP | ST-Link VCP | Serial2 reserved |
| UART 1 | PA9 | PA10 | |
| UART 3 | PB10 | PB11 | |
| UART 4 | PA0 | PA1 | |

### STM32 Nucleo F091RC

| UART | TX Pin | RX Pin | Notes |
|------|--------|--------|-------|
| Console | ST-Link VCP | ST-Link VCP | Serial2 reserved |
| UART 1 | PA9 | PA10 | |
| UART 3 | PB10 | PB11 | |
| UART 4 | PA0 | PA1 | |
| UART 5 | PB3 | PB4 | |
| UART 6 | PA4 | PA5 | |

### Raspberry Pi Pico

| UART | TX Pin | RX Pin | Notes |
|------|--------|--------|-------|
| Console | USB | USB | Primary serial |
| UART 1 | GP4 | GP5 | |
| UART 2 | GP8 | GP9 | |

### Arduino Mega 2560

| UART | TX Pin | RX Pin | Notes |
|------|--------|--------|-------|
| Console | USB | USB | Primary serial |
| UART 1 | 18 | 19 | |
| UART 2 | 16 | 17 | |
| UART 3 | 14 | 15 | |

### ESP32

| UART | TX Pin | RX Pin | Notes |
|------|--------|--------|-------|
| Console | USB | USB | Primary serial |
| UART 1 | GPIO17 | GPIO16 | |
| UART 2 | GPIO10 | GPIO9 | |

## Adding New Device Types

To add support for a new device:

1. Create a new directory under `src/devices/`
2. Implement the device state structure
3. Implement the protocol parser
4. Create a class implementing `IEmulatedDevice`
5. Create a factory class implementing `IDeviceFactory` (with `getCategory()`)
6. Register the factory in `main.cpp`

See `src/devices/yaesu/` (radio), `src/devices/g5500/` (rotator), or `src/devices/nmea_gps/` (GPS) for examples.

## References

- [Yaesu FT-991A CAT Manual (Official)](https://www.yaesu.com/Files/4CB893D7-1018-01AF-FA97E9E9AD48B50C/FT-991A_CAT_OM_ENG_1711-D.pdf)
- [Hamlib newcat.c](https://github.com/Derecho/hamlib/blob/master/yaesu/newcat.c) - Reference implementation
- [NMEA 0183 Standard](https://gpsd.gitlab.io/gpsd/NMEA.html) - GPS sentence format reference

## License

MIT License
