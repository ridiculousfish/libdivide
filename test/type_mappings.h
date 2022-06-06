#pragma once

#include <stdint.h>

#define LIB_DIVIDE_GENERATOR(GENERATOR, ...) \
    GENERATOR(uint16_t, u16, __VA_ARGS__) \
    GENERATOR(int16_t, s16, __VA_ARGS__) \
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

template <typename _IntT> struct struct_selector {};
#define DECLARE_STRUCT_SELECTOR(type, tag, ...) \
    template <> struct struct_selector<type> { \
        typedef libdivide::libdivide_ ##tag ##_t struct_t; \
        static const char * get_name() { return "libdivide_" #tag "_t"; } \
    };
LIB_DIVIDE_GENERATOR(DECLARE_STRUCT_SELECTOR, NULL)

template <typename _IntT>
static inline typename struct_selector<_IntT>::struct_t libdivide_gen(_IntT)
{
}
#define LIBDIVDE_GEN(type, tag, ...) \
    template <> \
    inline typename struct_selector<type>::struct_t libdivide_gen(type d) \
    { \
        return libdivide::libdivide_ ## tag ## _gen(d);\
    }
LIB_DIVIDE_GENERATOR(LIBDIVDE_GEN, NULL)
