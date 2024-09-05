#pragma once
#include "libdivide.h"

// constexpr auto constexprS16 = libdivide::libdivide_s16_gen(99);
// constexpr auto constexprS16BF = libdivide::libdivide_s16_branchfree_gen(99);
// constexpr auto constexprU16 = libdivide::libdivide_u16_gen(99);
// constexpr auto constexprU16BF = libdivide::libdivide_u16_branchfree_gen(99);
// constexpr auto constexprS32 = libdivide::libdivide_s32_gen(INT32_MAX/-123);
// constexpr auto constexprS32BF = libdivide::libdivide_s32_branchfree_gen(INT32_MAX/-123);
// constexpr auto constexprU32 = libdivide::libdivide_u32_gen(UINT32_MAX/123);
// constexpr auto constexprU32BF = libdivide::libdivide_u32_branchfree_gen(UINT32_MAX/123);
constexpr auto constexprS64 = libdivide::libdivide_s64_gen_c(INT64_MAX/-123);
constexpr auto constexprS64BF = libdivide::libdivide_s64_branchfree_gen_c(INT64_MAX/-123);
constexpr auto constexprU64 = libdivide::libdivide_u64_gen_c(UINT64_MAX/123);
constexpr auto constexprU64BF = libdivide::libdivide_u64_branchfree_gen_c(UINT64_MAX/123);

void test_constexpr(void);