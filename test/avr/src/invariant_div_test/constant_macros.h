#pragma once

#include <stdint.h>

#define CAT_HELPER(a, b) a ## b
#define CONCAT(A, B) CAT_HELPER(A, B)
#define TEST_FUNC_NAME(Op, Denom) CONCAT(CONCAT(test, Op), Denom)

#if TEST_UNSIGNED
typedef uint16_t test_t;
#define RANGE_MIN 0
#define RANGE_MAX UINT16_MAX-1
#else
typedef int16_t test_t;
#define RANGE_MIN INT16_MIN
#define RANGE_MAX INT16_MAX
#endif
#define LOOP_STEP 3U

// This is all about testing division by compile time constants
// So all tests have to be generated at compile time. Hence macros.
//
// This macro defines a common test function structure
#define TEST_FUNC(OPERATION, Op, Denom) \
int32_t TEST_FUNC_NAME(Op, Denom)() \
{ \
  int32_t checkSum = 0; \
  /* We need to be careful to have a wide enough range AND increment!=1 or else GCC figures out */ \
  /* this is a constant range and applies all sorts of optimizations */ \
  for (test_t loop = RANGE_MIN; loop < RANGE_MAX; loop+=LOOP_STEP) \
  { \
    checkSum += OPERATION(loop, Denom); \
  } \
    return checkSum; \
}

// Native test function
#if defined(TEST_DIV)
#define NATIVE_NAME NativeDiv
#define NATIVE_OP(Operand, Denom) ((test_t)Operand / (test_t)Denom)
#else
#define NATIVE_NAME NativeMod
#define NATIVE_OP(Operand, Denom) ((test_t)Operand % (test_t)Denom)
#endif
#define DEFINE_NATIVE_FUNC(Denom) TEST_FUNC(NATIVE_OP, NATIVE_NAME, Denom)
#define NATIVE_FUNC_NAME(Denom) TEST_FUNC_NAME(NATIVE_NAME, Denom)

#if defined(TEST_DIV)
#define LIBDIV_NAME LibDivDiv
#if TEST_CPP_TEMPLATE
#include "../../../constant_fast_div.hpp"
#define LIBDIV_OP(Operand, Denom) libdivide::fast_divide<test_t, Denom>(Operand)
#else
#include "../../../constant_fast_div.h"
#if TEST_UNSIGNED
#define LIBDIV_OP(Operand, Denom) FAST_DIV16U(Operand, Denom)
#else
#define LIBDIV_OP(Operand, Denom) FAST_DIV16(Operand, Denom)
#endif
#endif
#else
#include "../../../constant_fast_div.h"
#define LIBDIV_NAME LibDivMod
#define LIBDIV_OP(Operand, Denom) FAST_MOD16U(Operand, Denom)
#endif

#define DEFINE_LIBDIV_FUNC(Denom) TEST_FUNC(LIBDIV_OP, LIBDIV_NAME, Denom)
#define LIBDIV_FUNC_NAME(Denom) TEST_FUNC_NAME(LIBDIV_NAME, Denom)

// Below are all helper macros for the genrated code.

#if defined(TEST_DIV)
#if TEST_UNSIGNED
#if TEST_CPP_TEMPLATE
#define OP_NAME "DivUT"
#else
#define OP_NAME "DivU"
#endif
#else
#if TEST_CPP_TEMPLATE
#define OP_NAME "DivT"
#else
#define OP_NAME "Div"
#endif
#endif
#else
#define OP_NAME "Mod"
#endif

#define DEFINE_BOTH(Denom) DEFINE_NATIVE_FUNC(Denom); DEFINE_LIBDIV_FUNC(Denom);
#define RUN_TEST_BOTH(Denom) test_both(Denom, NATIVE_FUNC_NAME(Denom), LIBDIV_FUNC_NAME(Denom))