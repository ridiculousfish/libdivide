#pragma once

#if defined(__AVR__)

#include <Arduino.h>

namespace std
{
    template< class T >
    const T& (min)( const T& a, const T& b ) {
        return min(a, b);
    }
    template< class T >
    const T& (max)( const T& a, const T& b ) {
        return max(a, b);
    }
    template <typename _IntT>
    struct make_unsigned {
        typedef _IntT type;  
    };
    template <>
    struct make_unsigned<int16_t> {
        typedef uint16_t type;  
    };
    template <>
    struct make_unsigned<int32_t> {
        typedef uint32_t type;  
    };
    template <>
    struct make_unsigned<int64_t> {
        typedef uint64_t type;  
    };

    static const uint8_t CHAR_BIT = 8;

    template <typename _IntT>
    struct numeric_limits {
    };

    template <>
    struct numeric_limits<int16_t> {
        static constexpr int16_t (min)() { return INT16_MIN; }
        static constexpr int16_t (max)() { return INT16_MAX; }
        static const bool is_signed = true;
        static const int digits   = CHAR_BIT * sizeof(int16_t);
    };
    template <>
    struct numeric_limits<uint16_t> {
        static constexpr uint16_t (min)() { return 0; }
        static constexpr uint16_t (max)() { return UINT16_MAX; }
        static const bool is_signed = false;
        static const int digits   = CHAR_BIT * sizeof(uint16_t);
    };
    template <>
    struct numeric_limits<int32_t> {
        static constexpr int32_t (min)() { return INT32_MIN; }
        static constexpr int32_t (max)() { return INT32_MAX; }
        static const bool is_signed = true;
        static const int digits   = CHAR_BIT * sizeof(int32_t);
    };
    template <>
    struct numeric_limits<uint32_t> {
        static constexpr uint32_t (min)() { return 0; }
        static constexpr uint32_t (max)() { return UINT32_MAX; }
        static const bool is_signed = false;
        static const int digits   = CHAR_BIT * sizeof(uint32_t);
    };
    template <>
    struct numeric_limits<int64_t> {
        static constexpr int64_t (min)() { return INT64_MIN; }
        static constexpr int64_t (max)() { return INT64_MAX; }
        static const bool is_signed = true;
        static const int digits   = CHAR_BIT * sizeof(int64_t);
    };
    template <>
    struct numeric_limits<uint64_t> {
        static constexpr uint64_t (min)() { return 0; }
        static constexpr uint64_t (max)() { return UINT64_MAX; }
        static const bool is_signed = false;
        static const int digits   = CHAR_BIT * sizeof(uint64_t);
    };    
} // namespace std

#endif
