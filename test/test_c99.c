/*
 * A pure C test program. The point of this is to make sure libdivide
 * will compile as C only. 
 *
 * Since the other programs have CPP extensions, they will be compiled as C++. This
 * could allow C++ syntax or programming paradigms to inadvertently creep into the 
 * code base.  
 */

#include <stdio.h>
#include <inttypes.h>
#include "libdivide.h"
#include "constant_fast_div.h"

#if defined(_MSC_VER)
#pragma warning(disable : 4146)
#endif

#undef UNUSED
#define UNUSED(x) (void)(x)
#define MIN_RANGE (UINT16_MAX/4U)
#define LOOP_STEP 3
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define ABS(a)   MAX(-(a), a)

#define LOOP_START(denom) MIN(((denom*2)+LOOP_STEP), ((denom/2)+LOOP_STEP))
#define LOOP_END(type, denom, range_max) MIN(MAX((type)MIN_RANGE, ABS(denom)*4), range_max-(LOOP_STEP*2))
#define ASSERT_EQUAL(type, numer, denom, libdiv_result, native_result, format_spec) \
   if (libdiv_result!=native_result) { \
      fprintf(stderr, "Division fail: " #type ", %" format_spec "/%" format_spec ". Native: %" format_spec  ", Libdivide %" format_spec "\n", numer,  (type)denom, native_result, libdiv_result); \
   }
#define TEST_ONE(type, numer, denom, divider, format_spec, OPERATION) \
   type libdiv_result = OPERATION(numer, divider); \
   type native_result = numer / denom; \
   ASSERT_EQUAL(type, numer, denom, libdiv_result, native_result, format_spec)

#define TEST_BODY(type, range_max, denom, divider, format_spec, OPERATION) \
   /* We need to be careful to have a wide enough range AND increment!=1 or else GCC figures out */ \
   /* this is a constant range and applies all sorts of optimizations */ \
   { \
      type loop = (type)LOOP_START(denom); \
      const type end = (type)LOOP_END(type, denom, range_max); \
      const type step = MAX(LOOP_STEP, (end-loop)/(2<<12)); \
      printf("Testing " #type ", %" format_spec " from %" format_spec " to %" format_spec ", step %" format_spec "\n", (type)denom,  loop,  end, step); \
      for (; loop < end; loop+=step) \
      { \
         TEST_ONE(type, loop, denom, divider, format_spec, OPERATION) \
      } \
   }

void test_u16(void) {
#define U16_DENOM 953 // Prime
   struct libdivide_u16_t divider = libdivide_u16_gen(U16_DENOM);
#define OP_U16_DO(numer, divider) libdivide_u16_do(numer, &divider)
   TEST_BODY(uint16_t, UINT16_MAX, U16_DENOM, divider, PRIu16, OP_U16_DO)

#define CONSTANT_OP_U16(numer, denom) FAST_DIV16U(numer, denom)
   printf("Constant division ");
   TEST_BODY(uint16_t, UINT16_MAX, U16_DENOM, U16_DENOM, PRIu16, CONSTANT_OP_U16)
}

void test_s16(void) {
   int16_t denom = (int16_t)-4003;  // Prime
   struct libdivide_s16_t divider = libdivide_s16_gen(denom);
#define OP_S16(numer, divider) libdivide_s16_do(numer, &divider)
   TEST_BODY(int16_t, INT16_MAX, denom, divider, PRId16, OP_S16)

#define CONSTANT_OP_S16(numer, denom) FAST_DIV16(numer, denom)   
   printf("Constant division ");
   TEST_BODY(int16_t, INT16_MAX, 4003, 4003, PRId16, CONSTANT_OP_S16)

#define CONSTANT_OP_NEG_S16(numer, denom) FAST_DIV16_NEG(numer, denom)   
   printf("Constant division ");
   TEST_BODY(int16_t, INT16_MAX, -4003, 4003, PRId16, CONSTANT_OP_NEG_S16)  
}

void test_u32(void) {
   uint32_t denom = ((uint32_t)2 << 21) - 19; // Prime - see https://primes.utm.edu/lists/2small/0bit.html
   struct libdivide_u32_t divider = libdivide_u32_gen(denom);
#define OP_U32(numer, divider) libdivide_u32_do(numer, &divider)
   TEST_BODY(uint32_t, UINT32_MAX, denom, divider, PRIu32, OP_U32)
}

void test_s32(void) {
   int32_t denom = -(((int32_t)2 << 21) - 55); // Prime - see https://primes.utm.edu/lists/2small/0bit.html
   struct libdivide_s32_t divider = libdivide_s32_gen(denom);
#define OP_S32(numer, divider) libdivide_s32_do(numer, &divider)
   TEST_BODY(int32_t, INT32_MAX, denom, divider, PRId32, OP_S32)
}

void test_u64(void) {
   uint64_t denom = ((uint64_t)2 << 29) - 43;  // Prime - see https://primes.utm.edu/lists/2small/0bit.html
   struct libdivide_u64_t divider = libdivide_u64_gen(denom);
#define OP_U64(numer, divider) libdivide_u64_do(numer, &divider)
   TEST_BODY(uint64_t, (UINT64_MAX/2) /* For speed */, denom, divider, PRIu64, OP_U64)
}

void test_s64(void) {
   int64_t denom =  -(((int64_t)2 << 29) - 121); // Prime - see https://primes.utm.edu/lists/2small/0bit.html
   struct libdivide_s64_t divider = libdivide_s64_gen(denom);
#define OP_S64(numer, divider) libdivide_s64_do(numer, &divider)
   TEST_BODY(int64_t, INT64_MAX, denom, divider, PRId64, OP_S64)
}

int main (int argc, char *argv[]) { 
   UNUSED(argc);
   UNUSED(argv);
   
   test_u16();
   test_s16();
   test_u32();
   test_s32();
   test_u64();
   test_s64();

   return 0;
}
