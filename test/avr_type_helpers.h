#pragma once

#if defined(__AVR__)

#include <Arduino.h>

namespace std
{
    template <typename _IntT>
    struct make_unsigned {
        typedef _IntT type;  
    };
    template <>
    struct make_unsigned<uint16_t> {
        typedef int16_t type;  
    };
    template <>
    struct make_unsigned<uint32_t> {
        typedef int32_t type;  
    };
    template <>
    struct make_unsigned<uint64_t> {
        typedef int64_t type;  
    };

    static const uint8_t CHAR_BIT = 8;

    template <typename _IntT>
    struct numeric_limits {
    };

    template <>
    struct numeric_limits<int16_t> {
        static int16_t (min)() { return INT16_MIN; }
        static int16_t (max)() { return INT16_MAX; }
        static const bool is_signed = true;
        static const int digits   = CHAR_BIT * sizeof(int16_t);
    };
    template <>
    struct numeric_limits<uint16_t> {
        static uint16_t (min)() { return 0; }
        static uint16_t (max)() { return UINT16_MAX; }
        static const bool is_signed = false;
        static const int digits   = CHAR_BIT * sizeof(uint16_t);
    };
    template <>
    struct numeric_limits<int32_t> {
        static int32_t (min)() { return INT32_MIN; }
        static int32_t (max)() { return INT32_MAX; }
        static const bool is_signed = true;
        static const int digits   = CHAR_BIT * sizeof(int32_t);
    };
    template <>
    struct numeric_limits<uint32_t> {
        static uint32_t (min)() { return 0; }
        static uint32_t (max)() { return UINT32_MAX; }
        static const bool is_signed = false;
        static const int digits   = CHAR_BIT * sizeof(uint32_t);
    };
    template <>
    struct numeric_limits<int64_t> {
        static int64_t (min)() { return INT64_MIN; }
        static int64_t (max)() { return INT64_MAX; }
        static const bool is_signed = true;
        static const int digits   = CHAR_BIT * sizeof(int64_t);
    };
    template <>
    struct numeric_limits<uint64_t> {
        static uint64_t (min)() { return 0; }
        static uint64_t (max)() { return UINT64_MAX; }
        static const bool is_signed = false;
        static const int digits   = CHAR_BIT * sizeof(uint64_t);
    };    

    template <typename _IntT>
    struct is_unsigned {
        static const bool value = false;
    };
    template <>
    struct is_unsigned<uint16_t> {
        static const bool value = true;
    };
    template <>
    struct is_unsigned<uint32_t> {
        static const bool value = true;
    };
    template <>
    struct is_unsigned<uint64_t> {
        static const bool value = true;
    };
} // namespace std

#endif