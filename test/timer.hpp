#pragma once

#include <inttypes.h>

#if defined(_WIN32) || defined(WIN32)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <windows.h>
#elif defined(__AVR__)
#include <Arduino.h>
#else
#include <sys/time.h>
#endif

#define MILLIS_PER_SEC 1000ULL
#define MICROS_PER_SEC (MILLIS_PER_SEC*1000)
#define NANOS_PER_SEC (MICROS_PER_SEC*1000)

class timer {
private:
#if defined(LIBDIVIDE_WINDOWS)
    LARGE_INTEGER counter_freq;
    LARGE_INTEGER start_time;
    LARGE_INTEGER end_time;
#elif defined(__AVR__)
    uint64_t start_time;
    uint64_t end_time;
#else
    struct timeval start_time;
    struct timeval end_time;
#endif

public:

    timer() {
#if defined(LIBDIVIDE_WINDOWS)        
        QueryPerformanceFrequency(&counter_freq); 
#endif
    }

    void start() {
#if defined(LIBDIVIDE_WINDOWS)        
        QueryPerformanceCounter(&start_time);
#elif defined(__AVR__)
        start_time = micros();
#else 
        gettimeofday(&start_time, NULL);
#endif
    }

    void stop() {
#if defined(LIBDIVIDE_WINDOWS)        
        QueryPerformanceCounter(&end_time);
#elif defined(__AVR__)
        end_time = micros();
#else 
        gettimeofday(&end_time, NULL);
#endif        
    }

    uint64_t duration_nano() {
#if defined(LIBDIVIDE_WINDOWS)
        LARGE_INTEGER elapsed;
        elapsed.QuadPart = end_time.QuadPart - start_time.QuadPart;
        return (uint64_t)((elapsed.QuadPart * NANOS_PER_SEC) / counter_freq.QuadPart);
#elif defined(__AVR__)
        return (uint64_t)((end_time-start_time) * (NANOS_PER_SEC/MICROS_PER_SEC));
#else 
        return (uint64_t)(((end_time.tv_sec - start_time.tv_sec) * NANOS_PER_SEC) + ((end_time.tv_usec - start_time.tv_usec) * MICROS_PER_SEC));
#endif 
    }
};