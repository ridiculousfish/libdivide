#pragma once

#include <stdint.h>

#define LIB_DIVIDE_GENERATOR(GENERATOR, ...) \
    GENERATOR(uint32_t, u32, __VA_ARGS__) \
    GENERATOR(int32_t, s32, __VA_ARGS__) \
    GENERATOR(uint64_t, u64, __VA_ARGS__) \
    GENERATOR(int64_t, s64, __VA_ARGS__)


template <typename _IntT> struct type_tag {};
#define DECLARE_TAG_TYPE(type, tag, ...) \
    template <> struct type_tag<type> {\
        static const char * get_tag() { return #tag; } \
    };
LIB_DIVIDE_GENERATOR(DECLARE_TAG_TYPE, NULL)

template <typename _IntT> struct type_name {};
#define DECLARE_NAME_TYPE(type, tag, ...) \
    template <> struct type_name<type> {\
        static const char * get_name() { return #type; } \
    };
LIB_DIVIDE_GENERATOR(DECLARE_NAME_TYPE, NULL)