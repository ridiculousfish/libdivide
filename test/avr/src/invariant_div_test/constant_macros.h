#pragma once

#include "../../avr_type_helpers.h"

#define CAT_HELPER(a, b) a ## b
#define CONCAT(A, B) CAT_HELPER(A, B)
#define TEST_FUNC_NAME(Op, Denom) CONCAT(CONCAT(test, Op), Denom)

typedef uint16_t test_t;
constexpr test_t range = UINT16_MAX/4;
#define LOOP_STEP 3U

// This is all about testing division by compile time constants
// So all tests have to be generated at compile time. Hence macros.
//
// This macro defines a common test function structure
#define TEST_FUNC(OPERATION, Op, Denom) \
unsigned long TEST_FUNC_NAME(Op, Denom)(unsigned long checkSum) \
{ \
  /* We need to be careful to have a wide enough range AND increment!=1 or else GCC figures out */ \
  /* this is a constant range and applies all sorts of optimizations */ \
  test_t loop = (Denom/2U)+LOOP_STEP; \
  const test_t end = (test_t)min(max(range, ((uint32_t)Denom*4U)), (uint32_t)(std::numeric_limits<test_t>::max)()-(LOOP_STEP*2)); \
  for (; loop < end; loop+=LOOP_STEP) \
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

#include "../../../constant_fast_div.h"
#if defined(TEST_DIV)
#define LIBDIV_NAME LibDivDiv
#define LIBDIV_OP(Operand, Denom) FAST_DIV16U(Operand, Denom)
#else
#define LIBDIV_NAME LibDivMod
#define LIBDIV_OP(Operand, Denom) FAST_MOD16U(Operand, Denom)
#endif
#define DEFINE_LIBDIV_FUNC(Denom) TEST_FUNC(LIBDIV_OP, LIBDIV_NAME, Denom)
#define LIBDIV_FUNC_NAME(Denom) TEST_FUNC_NAME(LIBDIV_NAME, Denom)

// Below are all helper macros for the genrated code.

#if defined(TEST_DIV)
#define OP_NAME "Div"
#else
#define OP_NAME "Mod"
#endif

#define DEFINE_BOTH(Denom) DEFINE_NATIVE_FUNC(Denom); DEFINE_LIBDIV_FUNC(Denom);
#define RUN_TEST_BOTH(Denom) test_both(Denom, NATIVE_FUNC_NAME(Denom), LIBDIV_FUNC_NAME(Denom))