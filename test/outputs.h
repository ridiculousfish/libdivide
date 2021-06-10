#pragma once

#if defined(__AVR__)
#include <Arduino.h>

char *to_str(char *buffer, uint64_t n) {
    buffer += 20;
    *buffer-- = 0;
    while (n) {
        *buffer-- = (n % 10) + '0';
        n /= 10;
    }
    return buffer + 1;
}
char *to_str(char *buffer, int64_t n) {
    if (n<0){
        buffer = to_str(buffer+1, (uint64_t)(n*-1))-1;
        *buffer = '-';
        return buffer;
    }
    return to_str(buffer, (uint64_t)n);
}

template <typename _T>
void print_serial(const _T &item) { Serial.print(item); Serial.flush(); }
template <>
void print_serial(const uint64_t &item) 
{ 
    char buffer[32];
    Serial.print(to_str(buffer, item));
}
template <>
void print_serial(const int64_t &item) 
{ 
    char buffer[32];
    Serial.print(to_str(buffer, item));
}

#define PRINT_ERROR(item) print_serial(item)
#define PRINT_INFO(item) print_serial(item)

#else

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