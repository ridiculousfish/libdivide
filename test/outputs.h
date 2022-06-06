#pragma once

// Silence MSVC sprintf unsafe warnings
#define _CRT_SECURE_NO_WARNINGS

#include <inttypes.h>
#include <stdio.h>

#if defined(__AVR__)
#include <Arduino.h>

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
    if (n<0){
        buffer = to_str(buffer+1, (uint64_t)(n*-1))-1;
        *buffer = '-';
        return buffer;
    }
    return to_str(buffer, (uint64_t)n);
}

template <typename _T>
static inline void print_serial(const _T &item) { Serial.print(item); }
template <>
static inline void print_serial(const uint64_t &item)
{ 
    char buffer[32];
    Serial.print(to_str(buffer, item));
}
template <>
static inline void print_serial(const int64_t &item)
{ 
    char buffer[32];
    Serial.print(to_str(buffer, item));
}

#define PRINT_ERROR(item) print_serial(item)
#define PRINT_INFO(item) print_serial(item)

#else

static inline char *to_str(char *buffer, uint64_t n) {
    sprintf(buffer, "%" PRIu64, n);
    return buffer;
}
static inline char *to_str(char *buffer, int64_t n) {
    sprintf(buffer, "%" PRId64, n);
    return buffer;
}

#include <iostream>

#define PRINT_ERROR(item) std::cerr << item
#define PRINT_INFO(item) std::cout << item
#define F(item) item

#endif

#if defined(PRINT_DETAIL_PROGRESS)
#define PRINT_PROGRESS_MSG(item) PRINT_INFO(item)
#else
#define PRINT_PROGRESS_MSG(item)
#endif

static inline char *to_str(char *buffer, uint32_t n) {
    sprintf(buffer, "%" PRIu32, n);
    return buffer;
}
static inline char *to_str(char *buffer, int32_t n) {
    sprintf(buffer, "%" PRId32, n);
    return buffer;
}

static inline char *to_str(char *buffer, uint16_t n) {
    sprintf(buffer, "%" PRIu16, n);
    return buffer;
}
static inline char *to_str(char *buffer, int16_t n) {
    sprintf(buffer, "%" PRId16, n);
    return buffer;
}