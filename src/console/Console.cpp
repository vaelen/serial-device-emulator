// Copyright (c) 2025 Andrew C. Young <andrew@vaelen.org>
// SPDX-License-Identifier: MIT

#include "Console.h"
#include "ConfigStorage.h"
#include "devices/nmea_gps/NMEAGPSDevice.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

// Command table
static const ConsoleCommand commands[] = {
    {"help",    "help [command]",           "Show help for commands",               cmdHelp},
    {"types",   "types",                    "List available device types",          cmdTypes},
    {"uarts",   "uarts",                    "List available UARTs with pins",       cmdUarts},
    {"devices", "devices",                  "List active device instances",         cmdDevices},
    {"create",  "create <type> <uart>",     "Create device on UART (e.g., create radio 1)", cmdCreate},
    {"destroy", "destroy <id>",             "Destroy device by ID",                 cmdDestroy},
    {"start",   "start <id>",               "Start device",                         cmdStart},
    {"stop",    "stop <id>",                "Stop device",                          cmdStop},
    {"status",  "status [id]",              "Show device status",                   cmdStatus},
    {"options", "options <id>",             "List device options",                  cmdOptions},
    {"set",     "set <id> <option> <value>", "Set device option",                   cmdSet},
    {"get",     "get <id> <option>",        "Get device option value",              cmdGet},
    {"log",     "log <level>",              "Set log level (debug/info/warn/error)", cmdLog},
    {"smeter",  "smeter <id> <value>",      "Set S-meter value (0-15)",             cmdSmeter},
    {"power",   "power <id> <value>",       "Set power meter value",                cmdPower},
    {"swr",     "swr <id> <value>",         "Set SWR meter value",                  cmdSwr},
    {"save",    "save",                     "Save configuration to EEPROM",         cmdSave},
    {"clear",   "clear",                    "Clear stored configuration",           cmdClear},
    {"gps",     "gps <id> <lat> <lon> [alt]", "Set GPS position (decimal degrees)",  cmdGps},
    {nullptr, nullptr, nullptr, nullptr}
};

Console::Console(Stream& stream, DeviceManager& deviceMgr, ILogger& logger)
    : _stream(stream)
    , _deviceMgr(deviceMgr)
    , _logger(logger)
    , _cmdLen(0)
    , _echoEnabled(true)
{
    memset(_cmdBuffer, 0, sizeof(_cmdBuffer));
}

void Console::begin() {
    showWelcome();
    printPrompt();
}

void Console::update() {
    while (_stream.available()) {
        char c = _stream.read();

        // Handle backspace
        if (c == '\b' || c == 127) {
            if (_cmdLen > 0) {
                _cmdLen--;
                if (_echoEnabled) {
                    _stream.print("\b \b");
                }
            }
            continue;
        }

        // Handle enter
        if (c == '\r' || c == '\n') {
            if (_echoEnabled) {
                _stream.println();
            }
            if (_cmdLen > 0) {
                _cmdBuffer[_cmdLen] = '\0';
                processCommand();
                _cmdLen = 0;
            }
            printPrompt();
            continue;
        }

        // Handle escape sequences (arrow keys, etc.) - ignore
        if (c == 27) {
            // Skip next two chars of escape sequence
            while (_stream.available() < 2) { delay(1); }
            _stream.read();
            _stream.read();
            continue;
        }

        // Add printable characters to buffer
        if (c >= 32 && c < 127 && _cmdLen < COMMAND_BUFFER_SIZE - 1) {
            _cmdBuffer[_cmdLen++] = c;
            if (_echoEnabled) {
                _stream.print(c);
            }
        }
    }
}

void Console::processCommand() {
    char* argv[MAX_ARGS];
    int argc = parseArgs(_cmdBuffer, argv, MAX_ARGS);

    if (argc == 0) {
        return;
    }

    const ConsoleCommand* cmd = findCommand(argv[0]);
    if (cmd == nullptr) {
        printf("Unknown command: %s\n", argv[0]);
        println("Type 'help' for available commands.");
        return;
    }

    cmd->handler(*this, argc, argv);
}

int Console::parseArgs(char* line, char* argv[], int maxArgs) {
    int argc = 0;
    char* p = line;
    bool inQuote = false;

    while (*p && argc < maxArgs) {
        // Skip whitespace
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0') break;

        // Start of argument
        if (*p == '"') {
            inQuote = true;
            p++;
            argv[argc++] = p;
            while (*p && *p != '"') p++;
        } else {
            argv[argc++] = p;
            while (*p && *p != ' ' && *p != '\t') p++;
        }

        if (*p) {
            *p++ = '\0';
        }
    }

    return argc;
}

const ConsoleCommand* Console::findCommand(const char* name) {
    for (const ConsoleCommand* cmd = commands; cmd->name != nullptr; cmd++) {
        if (strcasecmp(cmd->name, name) == 0) {
            return cmd;
        }
    }
    return nullptr;
}

void Console::printPrompt() {
    _stream.print(CONSOLE_PROMPT);
}

void Console::showWelcome() {
    println();
    println("=================================");
    println("  Radio Emulator Console");
    printf("  Platform: %s\n", PLATFORM_NAME);
    printf("  Available UARTs: %d\n", PLATFORM_MAX_UARTS);
    print("  UARTs: ");
    for (uint8_t i = 1; i <= PLATFORM_MAX_UARTS; i++) {
        const char* pins = getUartPins(i);
        if (pins != nullptr) {
            if (i > 1) print(", ");
            printf("%d(%s)", i, pins);
        }
    }
    println();
    println("=================================");
    println("Type 'help' for available commands.");
    println();
}

void Console::print(const char* str) {
    _stream.print(str);
}

void Console::println(const char* str) {
    _stream.println(str);
}

void Console::printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(_outBuffer, sizeof(_outBuffer), fmt, args);
    va_end(args);
    _stream.print(_outBuffer);
}

// === Command Handlers ===

void cmdHelp(Console& console, int argc, char* argv[]) {
    if (argc > 1) {
        // Help for specific command
        for (const ConsoleCommand* cmd = commands; cmd->name != nullptr; cmd++) {
            if (strcasecmp(cmd->name, argv[1]) == 0) {
                console.printf("Usage: %s\n", cmd->usage);
                console.printf("  %s\n", cmd->help);
                return;
            }
        }
        console.printf("Unknown command: %s\n", argv[1]);
        return;
    }

    console.println("Available commands:");
    for (const ConsoleCommand* cmd = commands; cmd->name != nullptr; cmd++) {
        console.printf("  %-10s - %s\n", cmd->name, cmd->help);
    }
}

void cmdTypes(Console& console, int argc, char* argv[]) {
    DeviceManager& mgr = console.getDeviceManager();
    size_t count = mgr.getFactoryCount();

    if (count == 0) {
        console.println("No device types registered.");
        return;
    }

    // Display devices grouped by category
    const DeviceCategory categories[] = {
        DeviceCategory::RADIO,
        DeviceCategory::ROTATOR,
        DeviceCategory::GPS
    };

    console.println("Available device types:");

    for (size_t c = 0; c < 3; c++) {
        DeviceCategory cat = categories[c];
        bool hasDevices = false;

        // Check if there are any devices in this category
        for (size_t i = 0; i < count; i++) {
            const IDeviceFactory* factory = mgr.getFactory(i);
            if (factory->getCategory() == cat) {
                hasDevices = true;
                break;
            }
        }

        // Print category header
        console.printf("\n  %s:\n", categoryDisplayName(cat));

        if (!hasDevices) {
            console.println("    (none)");
            continue;
        }

        // Print devices in this category
        for (size_t i = 0; i < count; i++) {
            const IDeviceFactory* factory = mgr.getFactory(i);
            if (factory->getCategory() == cat) {
                console.printf("    %-12s - %s\n", factory->getTypeName(), factory->getDescription());
            }
        }
    }
}

void cmdDevices(Console& console, int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    DeviceManager& mgr = console.getDeviceManager();
    size_t count = mgr.getDeviceCount();

    if (count == 0) {
        console.println("No active devices.");
        return;
    }

    console.println("Active devices:");
    console.println("  ID  Type        UART  Pins              Status");
    console.println("  --  ----------  ----  ----------------  ------");

    for (uint8_t i = 0; i < MAX_DEVICES; i++) {
        IEmulatedDevice* dev = mgr.getDevice(i);
        if (dev != nullptr) {
            const char* pins = getUartPins(dev->getUartIndex());
            console.printf("  %2d  %-10s  %4d  %-16s  %s\n",
                          dev->getDeviceId(),
                          dev->getName(),
                          dev->getUartIndex(),
                          pins != nullptr ? pins : "N/A",
                          dev->isRunning() ? "running" : "stopped");
        }
    }
}

void cmdCreate(Console& console, int argc, char* argv[]) {
    if (argc < 3) {
        console.println("Usage: create <type> <uart>");
        return;
    }

    const char* typeName = argv[1];
    int uart = atoi(argv[2]);

    if (uart < 1 || uart > PLATFORM_MAX_UARTS) {
        console.printf("Invalid UART: %d (valid: 1-%d)\n", uart, PLATFORM_MAX_UARTS);
        return;
    }

    uint8_t deviceId = console.getDeviceManager().createDevice(typeName, uart);
    if (deviceId == 0xFF) {
        console.println("Failed to create device.");
    } else {
        console.printf("Created device %d\n", deviceId);
        // Auto-save configuration
        ConfigStorage::save(console.getDeviceManager());
    }
}

void cmdDestroy(Console& console, int argc, char* argv[]) {
    if (argc < 2) {
        console.println("Usage: destroy <id>");
        return;
    }

    int id = atoi(argv[1]);
    if (console.getDeviceManager().destroyDevice(id)) {
        console.printf("Destroyed device %d\n", id);
        // Auto-save configuration
        ConfigStorage::save(console.getDeviceManager());
    } else {
        console.printf("Failed to destroy device %d\n", id);
    }
}

void cmdStart(Console& console, int argc, char* argv[]) {
    if (argc < 2) {
        console.println("Usage: start <id>");
        return;
    }

    int id = atoi(argv[1]);
    IEmulatedDevice* dev = console.getDeviceManager().getDevice(id);
    if (dev == nullptr) {
        console.printf("Device %d not found\n", id);
        return;
    }

    if (dev->isRunning()) {
        console.println("Device is already running.");
        return;
    }

    if (dev->begin()) {
        console.printf("Started device %d\n", id);
    } else {
        console.println("Failed to start device.");
    }
}

void cmdStop(Console& console, int argc, char* argv[]) {
    if (argc < 2) {
        console.println("Usage: stop <id>");
        return;
    }

    int id = atoi(argv[1]);
    IEmulatedDevice* dev = console.getDeviceManager().getDevice(id);
    if (dev == nullptr) {
        console.printf("Device %d not found\n", id);
        return;
    }

    if (!dev->isRunning()) {
        console.println("Device is not running.");
        return;
    }

    dev->end();
    console.printf("Stopped device %d\n", id);
}

void cmdStatus(Console& console, int argc, char* argv[]) {
    DeviceManager& mgr = console.getDeviceManager();

    if (argc < 2) {
        // Show all devices
        cmdDevices(console, argc, argv);
        return;
    }

    int id = atoi(argv[1]);
    IEmulatedDevice* dev = mgr.getDevice(id);
    if (dev == nullptr) {
        console.printf("Device %d not found\n", id);
        return;
    }

    char statusBuf[256];
    dev->getStatus(statusBuf, sizeof(statusBuf));

    const char* pins = getUartPins(dev->getUartIndex());
    console.printf("Device %d (%s):\n", id, dev->getName());
    console.printf("  Description: %s\n", dev->getDescription());
    console.printf("  UART: %d (%s)\n", dev->getUartIndex(), pins != nullptr ? pins : "N/A");
    console.printf("  Status: %s\n", dev->isRunning() ? "running" : "stopped");
    console.println(statusBuf);
}

void cmdOptions(Console& console, int argc, char* argv[]) {
    if (argc < 2) {
        console.println("Usage: options <id>");
        return;
    }

    int id = atoi(argv[1]);
    IEmulatedDevice* dev = console.getDeviceManager().getDevice(id);
    if (dev == nullptr) {
        console.printf("Device %d not found\n", id);
        return;
    }

    size_t count = dev->getOptionCount();
    if (count == 0) {
        console.println("No configurable options.");
        return;
    }

    console.printf("Options for device %d:\n", id);
    char valBuf[64];
    for (size_t i = 0; i < count; i++) {
        const DeviceOption* opt = dev->getOption(i);
        formatOptionValue(*opt, valBuf, sizeof(valBuf));
        console.printf("  %-16s = %-12s  (%s)\n", opt->name, valBuf, opt->description);
    }
}

void cmdSet(Console& console, int argc, char* argv[]) {
    if (argc < 4) {
        console.println("Usage: set <id> <option> <value>");
        return;
    }

    int id = atoi(argv[1]);
    IEmulatedDevice* dev = console.getDeviceManager().getDevice(id);
    if (dev == nullptr) {
        console.printf("Device %d not found\n", id);
        return;
    }

    if (dev->setOption(argv[2], argv[3])) {
        console.printf("Set %s = %s\n", argv[2], argv[3]);
        // Auto-save configuration
        ConfigStorage::save(console.getDeviceManager());
    } else {
        console.printf("Failed to set %s\n", argv[2]);
    }
}

void cmdGet(Console& console, int argc, char* argv[]) {
    if (argc < 3) {
        console.println("Usage: get <id> <option>");
        return;
    }

    int id = atoi(argv[1]);
    IEmulatedDevice* dev = console.getDeviceManager().getDevice(id);
    if (dev == nullptr) {
        console.printf("Device %d not found\n", id);
        return;
    }

    char valBuf[64];
    if (dev->getOptionValue(argv[2], valBuf, sizeof(valBuf))) {
        console.printf("%s = %s\n", argv[2], valBuf);
    } else {
        console.printf("Unknown option: %s\n", argv[2]);
    }
}

void cmdLog(Console& console, int argc, char* argv[]) {
    ILogger& logger = console.getLogger();

    if (argc < 2) {
        console.printf("Current log level: %s\n", logLevelToString(logger.getLevel()));
        return;
    }

    LogLevel level;
    if (parseLogLevel(argv[1], level)) {
        logger.setLevel(level);
        console.printf("Log level set to: %s\n", logLevelToString(level));
    } else {
        console.println("Invalid level. Use: debug, info, warn, error");
    }
}

void cmdSmeter(Console& console, int argc, char* argv[]) {
    if (argc < 3) {
        console.println("Usage: smeter <id> <value>");
        return;
    }

    int id = atoi(argv[1]);
    int value = atoi(argv[2]);

    IEmulatedDevice* dev = console.getDeviceManager().getDevice(id);
    if (dev == nullptr) {
        console.printf("Device %d not found\n", id);
        return;
    }

    if (dev->setMeter(MeterType::SMETER, value)) {
        console.printf("S-meter set to %d\n", value);
    } else {
        console.println("Failed to set S-meter");
    }
}

void cmdPower(Console& console, int argc, char* argv[]) {
    if (argc < 3) {
        console.println("Usage: power <id> <value>");
        return;
    }

    int id = atoi(argv[1]);
    int value = atoi(argv[2]);

    IEmulatedDevice* dev = console.getDeviceManager().getDevice(id);
    if (dev == nullptr) {
        console.printf("Device %d not found\n", id);
        return;
    }

    if (dev->setMeter(MeterType::POWER, value)) {
        console.printf("Power meter set to %d\n", value);
    } else {
        console.println("Failed to set power meter");
    }
}

void cmdSwr(Console& console, int argc, char* argv[]) {
    if (argc < 3) {
        console.println("Usage: swr <id> <value>");
        return;
    }

    int id = atoi(argv[1]);
    int value = atoi(argv[2]);

    IEmulatedDevice* dev = console.getDeviceManager().getDevice(id);
    if (dev == nullptr) {
        console.printf("Device %d not found\n", id);
        return;
    }

    if (dev->setMeter(MeterType::SWR, value)) {
        console.printf("SWR meter set to %d\n", value);
    } else {
        console.println("Failed to set SWR meter");
    }
}

void cmdSave(Console& console, int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    if (ConfigStorage::save(console.getDeviceManager())) {
        console.println("Configuration saved to EEPROM.");
    } else {
        console.println("Failed to save configuration.");
    }
}

void cmdClear(Console& console, int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    ConfigStorage::clear();
    console.println("Configuration cleared from EEPROM.");
}

void cmdGps(Console& console, int argc, char* argv[]) {
    if (argc < 4) {
        console.println("Usage: gps <id> <lat> <lon> [alt]");
        console.println("  lat/lon in decimal degrees (e.g., 37.7749 -122.4194)");
        return;
    }

    int id = atoi(argv[1]);
    IEmulatedDevice* dev = console.getDeviceManager().getDevice(id);
    if (dev == nullptr) {
        console.printf("Device %d not found\n", id);
        return;
    }

    // Check if this is a GPS device
    if (strcmp(dev->getName(), "nmea-gps") != 0) {
        console.printf("Device %d is not a GPS device\n", id);
        return;
    }

    double lat = atof(argv[2]);
    double lon = atof(argv[3]);
    float alt = 0.0f;
    if (argc > 4) {
        alt = atof(argv[4]);
    }

    // Validate latitude/longitude ranges
    if (lat < -90.0 || lat > 90.0) {
        console.println("Invalid latitude (must be -90 to 90)");
        return;
    }
    if (lon < -180.0 || lon > 180.0) {
        console.println("Invalid longitude (must be -180 to 180)");
        return;
    }

    NMEAGPSDevice* gps = static_cast<NMEAGPSDevice*>(dev);
    gps->setPosition(lat, lon, alt);
    console.printf("GPS position set to %.6f, %.6f", lat, lon);
    if (argc > 4) {
        console.printf(", %.1fm", alt);
    }
    console.println();
}

void cmdUarts(Console& console, int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    console.printf("Available UARTs on %s:\n", PLATFORM_NAME);
    console.println("  UART  Pins              Status");
    console.println("  ----  ----------------  ----------");

    DeviceManager& mgr = console.getDeviceManager();

    for (uint8_t i = 1; i <= PLATFORM_MAX_UARTS; i++) {
        const char* pins = getUartPins(i);
        if (pins == nullptr) {
            continue;  // Skip UARTs without pin info
        }

        // Check if UART is available or in use
        const char* status = "available";
        for (uint8_t d = 0; d < MAX_DEVICES; d++) {
            IEmulatedDevice* dev = mgr.getDevice(d);
            if (dev != nullptr && dev->getUartIndex() == i) {
                static char statusBuf[24];
                snprintf(statusBuf, sizeof(statusBuf), "in use (dev %d)", dev->getDeviceId());
                status = statusBuf;
                break;
            }
        }

        console.printf("  %4d  %-16s  %s\n", i, pins, status);
    }
}
