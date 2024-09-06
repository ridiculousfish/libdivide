#pragma once
#include "libdivide.h"

// For constexpr, MSVC 17 has some unwanted warnings *at the call site*
#if defined(_MSC_VER)
#pragma warning(push)
// disable warning C4146: unary minus operator applied
// to unsigned type, result still unsigned
#pragma warning(disable : 4146)
// disable warning C4307: integral constant overflow
#pragma warning(disable : 4307)
#endif

constexpr auto constexprS16 = libdivide::libdivide_s16_gen_c(99);
constexpr auto constexprS16BF = libdivide::libdivide_s16_branchfree_gen_c(99);
constexpr auto constexprU16 = libdivide::libdivide_u16_gen_c(99);
constexpr auto constexprU16BF = libdivide::libdivide_u16_branchfree_gen_c(99);
constexpr auto constexprS32 = libdivide::libdivide_s32_gen_c(INT32_MAX/-123);
constexpr auto constexprS32BF = libdivide::libdivide_s32_branchfree_gen_c(INT32_MAX/-123);
constexpr auto constexprU32 = libdivide::libdivide_u32_gen_c(UINT32_MAX/123);
constexpr auto constexprU32BF = libdivide::libdivide_u32_branchfree_gen_c(UINT32_MAX/123);
constexpr auto constexprS64 = libdivide::libdivide_s64_gen_c(INT64_MAX/-123);
constexpr auto constexprS64BF = libdivide::libdivide_s64_branchfree_gen_c(INT64_MAX/-123);
constexpr auto constexprU64 = libdivide::libdivide_u64_gen_c(UINT64_MAX/123);
constexpr auto constexprU64BF = libdivide::libdivide_u64_branchfree_gen_c(UINT64_MAX/123);

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

void test_constexpr(void);