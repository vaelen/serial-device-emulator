// Copyright (c) 2025 Andrew C. Young <andrew@vaelen.org>
// SPDX-License-Identifier: MIT

#pragma once

#include <Arduino.h>

// Maximum length for string option values
#define OPTION_STRING_MAX_LEN 32
#define OPTION_ENUM_MAX_VALUES 8

enum class OptionType : uint8_t {
    UINT32,     // Numeric value with min/max range
    BOOL,       // Boolean true/false
    ENUM,       // Enumerated string values
    STRING      // Free-form string
};

// Configuration option descriptor
struct DeviceOption {
    const char* name;           // Option identifier (e.g., "baud_rate")
    const char* description;    // Human-readable description
    OptionType type;

    union {
        struct {
            uint32_t min;
            uint32_t max;
            uint32_t current;
        } uint32Val;

        bool boolVal;

        struct {
            const char* const* values;  // Array of valid values
            uint8_t count;              // Number of valid values
            uint8_t current;            // Current selection index
        } enumVal;

        char stringVal[OPTION_STRING_MAX_LEN];
    } value;
};

// Helper functions for creating options

inline DeviceOption makeUint32Option(const char* name, const char* desc,
                                      uint32_t min, uint32_t max, uint32_t current) {
    DeviceOption opt;
    opt.name = name;
    opt.description = desc;
    opt.type = OptionType::UINT32;
    opt.value.uint32Val.min = min;
    opt.value.uint32Val.max = max;
    opt.value.uint32Val.current = current;
    return opt;
}

inline DeviceOption makeBoolOption(const char* name, const char* desc, bool current) {
    DeviceOption opt;
    opt.name = name;
    opt.description = desc;
    opt.type = OptionType::BOOL;
    opt.value.boolVal = current;
    return opt;
}

inline DeviceOption makeEnumOption(const char* name, const char* desc,
                                    const char* const* values, uint8_t count, uint8_t current) {
    DeviceOption opt;
    opt.name = name;
    opt.description = desc;
    opt.type = OptionType::ENUM;
    opt.value.enumVal.values = values;
    opt.value.enumVal.count = count;
    opt.value.enumVal.current = current;
    return opt;
}

inline DeviceOption makeStringOption(const char* name, const char* desc, const char* current) {
    DeviceOption opt;
    opt.name = name;
    opt.description = desc;
    opt.type = OptionType::STRING;
    strncpy(opt.value.stringVal, current, OPTION_STRING_MAX_LEN - 1);
    opt.value.stringVal[OPTION_STRING_MAX_LEN - 1] = '\0';
    return opt;
}

// Format option value to string buffer, returns bytes written
inline size_t formatOptionValue(const DeviceOption& opt, char* buffer, size_t bufLen) {
    switch (opt.type) {
        case OptionType::UINT32:
            return snprintf(buffer, bufLen, "%lu", (unsigned long)opt.value.uint32Val.current);

        case OptionType::BOOL:
            return snprintf(buffer, bufLen, "%s", opt.value.boolVal ? "true" : "false");

        case OptionType::ENUM:
            if (opt.value.enumVal.current < opt.value.enumVal.count) {
                return snprintf(buffer, bufLen, "%s",
                               opt.value.enumVal.values[opt.value.enumVal.current]);
            }
            return snprintf(buffer, bufLen, "?");

        case OptionType::STRING:
            return snprintf(buffer, bufLen, "%s", opt.value.stringVal);

        default:
            return snprintf(buffer, bufLen, "?");
    }
}

// Parse string value and update option, returns true on success
inline bool parseOptionValue(DeviceOption& opt, const char* str) {
    switch (opt.type) {
        case OptionType::UINT32: {
            char* endptr;
            unsigned long val = strtoul(str, &endptr, 10);
            if (*endptr != '\0') return false;
            if (val < opt.value.uint32Val.min || val > opt.value.uint32Val.max) return false;
            opt.value.uint32Val.current = (uint32_t)val;
            return true;
        }

        case OptionType::BOOL:
            if (strcasecmp(str, "true") == 0 || strcmp(str, "1") == 0) {
                opt.value.boolVal = true;
                return true;
            }
            if (strcasecmp(str, "false") == 0 || strcmp(str, "0") == 0) {
                opt.value.boolVal = false;
                return true;
            }
            return false;

        case OptionType::ENUM:
            for (uint8_t i = 0; i < opt.value.enumVal.count; i++) {
                if (strcasecmp(str, opt.value.enumVal.values[i]) == 0) {
                    opt.value.enumVal.current = i;
                    return true;
                }
            }
            return false;

        case OptionType::STRING:
            strncpy(opt.value.stringVal, str, OPTION_STRING_MAX_LEN - 1);
            opt.value.stringVal[OPTION_STRING_MAX_LEN - 1] = '\0';
            return true;

        default:
            return false;
    }
}
