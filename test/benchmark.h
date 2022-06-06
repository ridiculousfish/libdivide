// Shared bench marking code
#pragma once

#include <inttypes.h>
#include <stdio.h>
#if defined(__AVR__)
#include "avr_type_helpers.h"
#else
#include <algorithm>
#include <limits>
#include <type_traits>
#endif

#undef UNUSED
#define UNUSED(x) (void)(x)

#if defined(_WIN32) || defined(WIN32)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <windows.h>
#define LIBDIVIDE_WINDOWS
#endif

#include "libdivide.h"
#include "outputs.h"
#include "random_numerators.hpp"
#include "timer.hpp"
#include "type_mappings.h"

using namespace libdivide;

#if defined(__GNUC__)
#define NOINLINE __attribute__((__noinline__))
#elif defined(_MSC_VER)
#define NOINLINE __declspec(noinline)
#pragma warning(disable : 4146)
#else
#define NOINLINE
#endif

#if defined(LIBDIVIDE_AVX512)
#define x86_VECTOR_TYPE __m512i
#define SETZERO_SI _mm512_setzero_si512
#define LOAD_SI _mm512_load_si512
#define ADD_EPI64 _mm512_add_epi64
#define ADD_EPI32 _mm512_add_epi32
#define ADD_EPI16 _mm512_add_epi16
#elif defined(LIBDIVIDE_AVX2)
#define x86_VECTOR_TYPE __m256i
#define SETZERO_SI _mm256_setzero_si256
#define LOAD_SI _mm256_load_si256
#define ADD_EPI64 _mm256_add_epi64
#define ADD_EPI32 _mm256_add_epi32
#define ADD_EPI16 _mm256_add_epi16
#elif defined(LIBDIVIDE_SSE2)
#define x86_VECTOR_TYPE __m128i
#define SETZERO_SI _mm_setzero_si128
#define LOAD_SI _mm_load_si128
#define ADD_EPI64 _mm_add_epi64
#define ADD_EPI32 _mm_add_epi32
#define ADD_EPI16 _mm_add_epi16
#endif

// Helper - given a vector of some type, convert it to unsigned and sum it.
// This is factored out in this funny way to avoid signed integer overflow.
template <typename IntT>
inline uint64_t unsigned_sum_vals(const IntT *vals, size_t count) {
    typedef typename std::make_unsigned<IntT>::type UIntT;
    UIntT sum = 0;
    for (size_t i = 0; i < count; i++) {
        sum += static_cast<UIntT>(vals[i]);
    }
    return sum;
}

template <typename IntT, typename Divisor>
NOINLINE uint64_t sum_quotients(const random_numerators<IntT> &vals, const Divisor &div) {
    // Need to use unsigned to avoid signed integer overlow.
    typedef typename std::make_unsigned<IntT>::type UIntT;
    UIntT sum = 0;
    for (auto iter = vals.begin(); iter != vals.end(); ++iter) {
        sum += (UIntT)(*iter / div);
    }
    return (uint64_t)sum;
}

#ifdef x86_VECTOR_TYPE

template <size_t IntSize>
inline x86_VECTOR_TYPE add_vector(x86_VECTOR_TYPE sumX4, x86_VECTOR_TYPE numers) {
    UNUSED(numers);
    abort();
    return sumX4;
}
template <>
inline x86_VECTOR_TYPE add_vector<2U>(x86_VECTOR_TYPE sumX4, x86_VECTOR_TYPE numers) {
    return ADD_EPI16(sumX4, numers);
}
template <>
inline x86_VECTOR_TYPE add_vector<4U>(x86_VECTOR_TYPE sumX4, x86_VECTOR_TYPE numers) {
    return ADD_EPI32(sumX4, numers);
}
template <>
inline x86_VECTOR_TYPE add_vector<8U>(x86_VECTOR_TYPE sumX4, x86_VECTOR_TYPE numers) {
    return ADD_EPI64(sumX4, numers);
}

template <typename IntT, typename Divisor>
NOINLINE uint64_t sum_quotients_vec(const random_numerators<IntT> &vals, const Divisor &div) {
    size_t count = sizeof(x86_VECTOR_TYPE) / sizeof(IntT);
    x86_VECTOR_TYPE sumX4 = SETZERO_SI();
    for (auto iter = vals.begin(); iter != vals.end(); iter += count) {
        x86_VECTOR_TYPE numers = LOAD_SI((const x86_VECTOR_TYPE *)iter);
        numers = numers / div;
        sumX4 = add_vector<sizeof(IntT)>(sumX4, numers);
    }
    return unsigned_sum_vals((const IntT *)&sumX4, count);
}

#elif defined(LIBDIVIDE_NEON)

// Helper to deduce NEON vector type for integral type.
template <typename T>
struct NeonVecFuncs {};

template <>
struct NeonVecFuncs<uint16_t> {
    static inline uint16x8_t dup(uint16_t value) { return vdupq_n_u16(value); }
    static inline uint16x8_t add(uint16x8_t a, uint16x8_t b) { return vaddq_u16(a, b); }
};

template <>
struct NeonVecFuncs<int16_t> {
    static inline int16x8_t dup(int16_t value) { return vdupq_n_s16(value); }
    static inline int16x8_t add(int16x8_t a, int16x8_t b) { return vaddq_s16(a, b); }
};

template <>
struct NeonVecFuncs<uint32_t> {
    static inline uint32x4_t dup(uint32_t value) { return vdupq_n_u32(value); }
    static inline uint32x4_t add(uint32x4_t a, uint32x4_t b) { return vaddq_u32(a, b); }
};

template <>
struct NeonVecFuncs<int32_t> {
    static inline int32x4_t dup(int32_t value) { return vdupq_n_s32(value); }
    static inline int32x4_t add(int32x4_t a, int32x4_t b) { return vaddq_s32(a, b); }
};

template <>
struct NeonVecFuncs<uint64_t> {
    static inline uint64x2_t dup(uint64_t value) { return vdupq_n_u64(value); }
    static inline uint64x2_t add(uint64x2_t a, uint64x2_t b) { return vaddq_u64(a, b); }
};

template <>
struct NeonVecFuncs<int64_t> {
    static inline int64x2_t dup(int64_t value) { return vdupq_n_s64(value); }
    static inline int64x2_t add(int64x2_t a, int64x2_t b) { return vaddq_s64(a, b); }
};

template <typename IntT, typename Divisor>
NOINLINE uint64_t sum_quotients_vec(const random_numerators<IntT> &vals, const Divisor &div) {
    typedef typename NeonVecFor<IntT>::type NeonVectorType;
    size_t count = sizeof(NeonVectorType) / sizeof(IntT);
    NeonVectorType sumX4 = NeonVecFuncs<IntT>::dup(0);
    for (auto iter = vals.begin(); iter != vals.end(); iter += count) {
        NeonVectorType numers = *(NeonVectorType *)iter;
        numers = numers / div;
        sumX4 = NeonVecFuncs<IntT>::add(sumX4, numers);
    }
    return unsigned_sum_vals((const IntT *)&sumX4, count);
}

#endif

// noinline to force compiler to emit this
template <typename IntT>
NOINLINE divider<IntT> generate_1_divisor(IntT d) {
    return divider<IntT>(d);
}

template <typename IntT>
NOINLINE void generate_divisor(const random_numerators<IntT> &vals, IntT denom) {
    for (size_t iter = 0; iter < vals.length(); iter++) {
        (void)generate_1_divisor(denom);
    }
}

struct time_double {
    uint64_t time;  // in nanoseconds
    uint64_t result;
};

template <typename IntT, class DenomT>
using pFuncToTime = uint64_t (*)(const random_numerators<IntT> &, const DenomT &);

template <typename IntT, class DenomT>
NOINLINE static time_double time_function(
    const random_numerators<IntT> &vals, DenomT denom, pFuncToTime<IntT, DenomT> timeFunc) {
    time_double tresult;

    timer t;
    t.start();
    tresult.result = timeFunc(vals, denom);
    t.stop();
    tresult.time = t.duration_nano();
    return tresult;
}

struct TestResult {
    double hardware_time;
    double base_time;
    double branchfree_time;
    double vector_time;
    double vector_branchfree_time;
    double gen_time;
    int algo;
};

#define TEST_COUNT 30

inline void check_result(uint64_t expected, uint64_t actual, uint32_t line_no) {
    if ((actual) != (expected)) {
        PRINT_ERROR("Failure on line ");
        PRINT_ERROR(line_no);
        PRINT_ERROR("\n");
    }
}

template <typename IntT>
NOINLINE TestResult test_one(const random_numerators<IntT> &vals, IntT denom) {
    const bool testBranchfree = (denom != 1);
    divider<IntT, BRANCHFULL> div_bfull(denom);
    divider<IntT, BRANCHFREE> div_bfree(testBranchfree ? denom : 2);

    uint64_t min_my_time = INT64_MAX, min_my_time_branchfree = INT64_MAX,
             min_my_time_vector = INT64_MAX, min_my_time_vector_branchfree = INT64_MAX,
             min_his_time = INT64_MAX, min_gen_time = INT64_MAX;
    time_double tresult;
    for (size_t iter = 0; iter < TEST_COUNT; iter++) {
        tresult = time_function(vals, denom, sum_quotients);
        min_his_time = (std::min)(min_his_time, tresult.time);
        const uint64_t expected = tresult.result;

        tresult = time_function(vals, div_bfull, sum_quotients);
        min_my_time = (std::min)(min_my_time, tresult.time);
        check_result(tresult.result, expected, __LINE__);

        if (testBranchfree) {
            tresult = time_function(vals, div_bfree, sum_quotients);
            min_my_time_branchfree = (std::min)(min_my_time_branchfree, tresult.time);
            check_result(tresult.result, expected, __LINE__);
        }

#if defined(x86_VECTOR_TYPE) || defined(LIBDIVIDE_NEON)
        tresult = time_function(vals, div_bfull, sum_quotients_vec);
        min_my_time_vector = (std::min)(min_my_time_vector, tresult.time);
        check_result(tresult.result, expected, __LINE__);

        if (testBranchfree) {
            tresult = time_function(vals, div_bfree, sum_quotients_vec);
            min_my_time_vector_branchfree = (std::min)(min_my_time_vector_branchfree, tresult.time);
            check_result(tresult.result, expected, __LINE__);
        }
#else
        min_my_time_vector = 0;
        min_my_time_vector_branchfree = 0;
#endif

        {
            timer t;
            t.start();
            generate_divisor(vals, denom);
            t.stop();
            min_gen_time = (std::min)(min_gen_time, t.duration_nano());
        }
    }

    TestResult result;
    result.gen_time = min_gen_time / (double)vals.length();
    result.base_time = min_my_time / (double)vals.length();
    result.branchfree_time = testBranchfree ? min_my_time_branchfree / (double)vals.length() : -1;
    result.vector_time = min_my_time_vector / (double)vals.length();
    result.vector_branchfree_time =
        testBranchfree ? min_my_time_vector_branchfree / (double)vals.length() : -1;
    result.hardware_time = min_his_time / (double)vals.length();
    return result;
}

template <typename _IntT>
int32_t get_algorithm(_IntT d) {
    const auto denom = libdivide_gen(d);
    uint8_t more = denom.more;
    if (!denom.magic)
        return 0;
    else if (!(more & LIBDIVIDE_ADD_MARKER))
        return 1;
    else
        return 2;
}

template <typename _IntT>
NOINLINE TestResult test_one(_IntT d, const random_numerators<_IntT> &data) {
    struct TestResult result = test_one(data, d);
    result.algo = get_algorithm(d);
    return result;
}

// Result column width
#define PRIcw "10"

inline static void print_report_header(void) {
    char buffer[256];
    sprintf(buffer,
        "%6s %" PRIcw "s %" PRIcw "s %" PRIcw "s %" PRIcw "s %" PRIcw "s %" PRIcw "s %6s\n", "#",
        "system", "scalar", "scl_bf", "vector", "vec_bf", "gener", "algo");
    PRINT_INFO(buffer);
}

// Result column format spec
#define PRIrc PRIcw ".3f"

template <typename _IntT>
static void print_report_result(_IntT d, struct TestResult result) {
    char denom_buff[32];
    char *pDenom = to_str(denom_buff, d);

    char report_buff[256];
    sprintf(report_buff,
        "%6s %" PRIrc " %" PRIrc " %" PRIrc " %" PRIrc " %" PRIrc " %" PRIrc " %4d\n", pDenom,
        result.hardware_time, result.base_time, result.branchfree_time, result.vector_time,
        result.vector_branchfree_time, result.gen_time, result.algo);
    PRINT_INFO(report_buff);
}

template <typename _IntT>
static void print_banner() {
    char type_buffer[64];
    sprintf(type_buffer, "=== libdivide %s benchmark ===", type_tag<_IntT>::get_tag());
    char buffer[128];
    sprintf(buffer, "\n%50s\n\n", type_buffer);
    PRINT_INFO(buffer);
}

template <typename _IntT>
void test_many() {
    print_banner<_IntT>();
    print_report_header();
    random_numerators<_IntT> data;
    _IntT d = 1;
    while (true) {
        print_report_result(d, test_one(d, data));

        if (std::numeric_limits<_IntT>::is_signed) {
            d = -d;
            if (d > 0) d++;
        } else {
            d++;
        }
    }
}