#pragma once

// Silence MSVC sprintf unsafe warnings
#define _CRT_SECURE_NO_WARNINGS

#include <inttypes.h>
#include <stdio.h>

#if defined(__AVR__)
#include <Arduino.h>
#if defined(UNIT_TEST)
#include <unity.h>
#endif

// AVR doesn't support (s)printf() of 64-bit numbers.
// PRId64 is undefined & GCC will issue a warning
// So manually convert
static inline char *to_str(char *buffer, uint64_t n) {
    buffer += 20;
    *buffer-- = 0;
    while (n) {
        *buffer-- = (n % 10) + '0';
        n /= 10;
    }
    return buffer + 1;
}
static inline char *to_str(char *buffer, int64_t n) {
    if (n < 0) {
        buffer = to_str(buffer + 1, (uint64_t)(n * -1)) - 1;
        *buffer = '-';
        return buffer;
    }
    return to_str(buffer, (uint64_t)n);
}


static inline char *to_str(char *buffer, uint32_t n) {
    snprintf(buffer, 32, "%" PRIu32, n);
    return buffer;
}
static inline char *to_str(char *buffer, int32_t n) {
    snprintf(buffer, 32, "%" PRId32, n);
    return buffer;
}

static inline char *to_str(char *buffer, uint16_t n) {
    snprintf(buffer, 32, "%" PRIu16, n);
    return buffer;
}
static inline char *to_str(char *buffer, int16_t n) {
    snprintf(buffer, 32, "%" PRId16, n);
    return buffer;
}

#if defined(UNIT_TEST)
    void print_serial(const char *msg) {
        if (strcmp(msg, "\n") != 0) {
            TEST_MESSAGE(msg);
        }
    }
    void print_serial(char *msg) {
        if (strcmp(msg, "\n") != 0) {
            TEST_MESSAGE(msg);
        }
    }
#else
    void print_serial(const char *msg) {
        Serial.print(msg);
    }
    void print_serial(char *msg) {
        Serial.print(msg);
    }
#endif

void print_serial(const __FlashStringHelper *item) {
    char buffer[64];
    strncpy_P(buffer, (const char*)item, sizeof(buffer) - 1);
    print_serial(buffer);
}

void print_serial(const String &item) {
    print_serial(item.c_str());
}

template <typename _T>
void print_serial(const _T &item) {
    char buffer[32];
    print_serial(to_str(buffer, item));
}

#define PRINT_ERROR(item) print_serial(item)
#define PRINT_INFO(item) print_serial(item)
#if !defined(UNIT_TEST)
#define TEST_FAIL() exit(1)
#endif

#else

#include <iostream>

#define PRINT_ERROR(item) std::cerr << item
#define PRINT_INFO(item) std::cout << item
#define F(item) item
#define TEST_FAIL() exit(1)

static inline char *to_str(char *buffer, uint64_t n) {
    snprintf(buffer, 32, "%" PRIu64, n);
    return buffer;
}
static inline char *to_str(char *buffer, int64_t n) {
    snprintf(buffer, 32, "%" PRId64, n);
    return buffer;
}
static inline char *to_str(char *buffer, uint32_t n) {
    snprintf(buffer, 32, "%" PRIu32, n);
    return buffer;
}
static inline char *to_str(char *buffer, int32_t n) {
    snprintf(buffer, 32, "%" PRId32, n);
    return buffer;
}
static inline char *to_str(char *buffer, uint16_t n) {
    snprintf(buffer, 32, "%" PRIu16, n);
    return buffer;
}
static inline char *to_str(char *buffer, int16_t n) {
    snprintf(buffer, 32, "%" PRId16, n);
    return buffer;
}

#endif

#if defined(PRINT_DETAIL_PROGRESS)
#define PRINT_PROGRESS_MSG(item) PRINT_INFO(item)
#else
#define PRINT_PROGRESS_MSG(item)
#endif
