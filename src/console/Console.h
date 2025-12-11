// Copyright (c) 2025 Andrew C. Young <andrew@vaelen.org>
// SPDX-License-Identifier: MIT

#pragma once

#include <Arduino.h>
#include "platform_config.h"
#include "DeviceManager.h"
#include "ILogger.h"

// Maximum number of command arguments
#define MAX_ARGS 8

// Console command handler function type
class Console;
typedef void (*CommandHandler)(Console& console, int argc, char* argv[]);

// Command definition
struct ConsoleCommand {
    const char* name;
    const char* usage;
    const char* help;
    CommandHandler handler;
};

// Interactive command-line console
class Console {
public:
    Console(Stream& stream, DeviceManager& deviceMgr, ILogger& logger);

    // Initialize console
    void begin();

    // Process any available input (non-blocking)
    void update();

    // Output methods
    void print(const char* str);
    void println(const char* str = "");
    void printf(const char* fmt, ...);

    // Get references for command handlers
    Stream& getStream() { return _stream; }
    DeviceManager& getDeviceManager() { return _deviceMgr; }
    ILogger& getLogger() { return _logger; }

private:
    Stream& _stream;
    DeviceManager& _deviceMgr;
    ILogger& _logger;

    char _cmdBuffer[COMMAND_BUFFER_SIZE];
    size_t _cmdLen;
    bool _echoEnabled;

    // Output buffer for printf
    char _outBuffer[LOG_BUFFER_SIZE];

    // Process a complete command line
    void processCommand();

    // Parse command line into arguments
    int parseArgs(char* line, char* argv[], int maxArgs);

    // Find command by name
    const ConsoleCommand* findCommand(const char* name);

    // Print prompt
    void printPrompt();

    // Show welcome message
    void showWelcome();
};

// Built-in command handlers
void cmdHelp(Console& console, int argc, char* argv[]);
void cmdTypes(Console& console, int argc, char* argv[]);
void cmdDevices(Console& console, int argc, char* argv[]);
void cmdCreate(Console& console, int argc, char* argv[]);
void cmdDestroy(Console& console, int argc, char* argv[]);
void cmdStart(Console& console, int argc, char* argv[]);
void cmdStop(Console& console, int argc, char* argv[]);
void cmdStatus(Console& console, int argc, char* argv[]);
void cmdOptions(Console& console, int argc, char* argv[]);
void cmdSet(Console& console, int argc, char* argv[]);
void cmdGet(Console& console, int argc, char* argv[]);
void cmdLog(Console& console, int argc, char* argv[]);
void cmdSmeter(Console& console, int argc, char* argv[]);
void cmdPower(Console& console, int argc, char* argv[]);
void cmdSwr(Console& console, int argc, char* argv[]);
void cmdSave(Console& console, int argc, char* argv[]);
void cmdClear(Console& console, int argc, char* argv[]);
void cmdGps(Console& console, int argc, char* argv[]);
void cmdUarts(Console& console, int argc, char* argv[]);
