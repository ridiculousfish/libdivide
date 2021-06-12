// Shared bench marking code
#pragma once

#include <inttypes.h>
#include <stdio.h>
#if defined(__AVR__)
#include "avr_type_helpers.h"
#else
#include <type_traits>
#endif

#if defined(_WIN32) || defined(WIN32)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <windows.h>
#define LIBDIVIDE_WINDOWS
#endif

#include "..\libdivide.h"
#include "type_mappings.h"
#include "outputs.h"
#include "timer.hpp"
#include "random_data.hpp"

using namespace libdivide;

#if defined(__GNUC__)
#define NOINLINE __attribute__((__noinline__))
#elif defined(_MSC_VER)
#define NOINLINE __declspec(noinline)
#else
#define NOINLINE
#endif

#if defined(LIBDIVIDE_AVX512)
#define x86_VECTOR_TYPE __m512i
#define SETZERO_SI _mm512_setzero_si512
#define LOAD_SI _mm512_load_si512
#define ADD_EPI64 _mm512_add_epi64
#define ADD_EPI32 _mm512_add_epi32
#elif defined(LIBDIVIDE_AVX2)
#define x86_VECTOR_TYPE __m256i
#define SETZERO_SI _mm256_setzero_si256
#define LOAD_SI _mm256_load_si256
#define ADD_EPI64 _mm256_add_epi64
#define ADD_EPI32 _mm256_add_epi32
#elif defined(LIBDIVIDE_SSE2)
#define x86_VECTOR_TYPE __m128i
#define SETZERO_SI _mm_setzero_si128
#define LOAD_SI _mm_load_si128
#define ADD_EPI64 _mm_add_epi64
#define ADD_EPI32 _mm_add_epi32
#endif


volatile uint64_t sGlobalUInt64;

// Helper - given a vector of some type, convert it to unsigned and sum it
template <typename IntT>
inline uint64_t unsigned_sum_vals(const IntT *vals, size_t count) {
    uint64_t sum = 0;
    for (size_t i=0; i < count; i++) {
        sum += static_cast<uint64_t>(vals[i]);
    }
    return sum;
}

template <typename IntT, typename Divisor>
NOINLINE uint64_t sum_quotients(const random_data<IntT> &vals, Divisor div) {
    uint64_t sum = 0;
    auto end = vals.end();
    for (auto pNumerator = vals.begin(); pNumerator != end; pNumerator++) {
        sum += (uint64_t)(*pNumerator / div);
    }
    return sum;
}

#ifdef x86_VECTOR_TYPE
template <typename IntT, typename Divisor>
NOINLINE uint64_t sum_quotients_vec(const random_data<IntT> &vals, Divisor div) {
    size_t count = sizeof(x86_VECTOR_TYPE) / sizeof(IntT);
    x86_VECTOR_TYPE sumX4 = SETZERO_SI();
    for (auto iter = vals.begin(); iter != vals.end(); iter += count) {
        x86_VECTOR_TYPE numers = LOAD_SI((const x86_VECTOR_TYPE *)iter);
        numers = numers / div;
        if (sizeof(IntT) == 4) {
            sumX4 = ADD_EPI32(sumX4, numers);
        } else if (sizeof(IntT) == 8) {
            sumX4 = ADD_EPI64(sumX4, numers);
        } else {
            abort();
        }
    }
    return unsigned_sum_vals((const IntT *)&sumX4, count);
}
#elif defined(LIBDIVIDE_NEON)

template <typename Divisor>
NOINLINE uint64_t sum_quotients_vec(const random_data<uint32_t> &vals, Divisor div) {
    typedef uint32_t IntT;
    typedef typename NeonVecFor<IntT>::type NeonVectorType;
    size_t count = sizeof(NeonVectorType) / sizeof(IntT);
    NeonVectorType sumX4 = vdupq_n_u32(0);
    for (random_data<uint32_t>::const_iterator iter = vals.begin(); iter != vals.end(); iter += count) {
        NeonVectorType numers = *(NeonVectorType *)iter;
        numers = numers / div;
        sumX4 = vaddq_u32(sumX4, numers);
    }
    return unsigned_sum_vals((const IntT *)&sumX4, count);
}

template <typename Divisor>
NOINLINE uint64_t sum_quotients_vec(const random_data<int32_t> &vals, Divisor div) {
    typedef int32_t IntT;
    typedef typename NeonVecFor<IntT>::type NeonVectorType;
    size_t count = sizeof(NeonVectorType) / sizeof(IntT);
    NeonVectorType sumX4 = vdupq_n_s32(0);
    for (random_data<int32_t>::const_iterator iter = vals.begin(); iter != vals.end(); iter += count) {
        NeonVectorType numers = *(NeonVectorType *)iter;
        numers = numers / div;
        sumX4 = vaddq_s32(sumX4, numers);
    }
    return unsigned_sum_vals((const IntT *)&sumX4, count);
}

template <typename Divisor>
NOINLINE uint64_t sum_quotients_vec(const random_data<uint64_t> &vals, Divisor div) {
    typedef uint64_t IntT;
    typedef typename NeonVecFor<IntT>::type NeonVectorType;
    size_t count = sizeof(NeonVectorType) / sizeof(IntT);
    NeonVectorType sumX4 = vdupq_n_u64(0);
    for (random_data<uint64_t>::const_iterator iter = vals.begin(); iter != vals.end(); iter += count) {
        NeonVectorType numers = *(NeonVectorType *)iter;
        numers = numers / div;
        sumX4 = vaddq_u64(sumX4, numers);
    }
    return unsigned_sum_vals((const IntT *)&sumX4, count);
}

template <typename Divisor>
NOINLINE uint64_t sum_quotients_vec(const random_data<int64_t> &vals, Divisor div) {
    typedef int64_t IntT;
    typedef typename NeonVecFor<IntT>::type NeonVectorType;
    size_t count = sizeof(NeonVectorType) / sizeof(IntT);
    const uint64x2_t zeros = vdupq_n_u64(0);
    NeonVectorType sumX4 = *reinterpret_cast<const NeonVectorType *>(&zeros);
    for (random_data<int64_t>::const_iterator iter = vals.begin(); iter != vals.end(); iter += count) {
        NeonVectorType numers = *(NeonVectorType *)iter;
        numers = numers / div;
        sumX4 = vaddq_s64(sumX4, numers);
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
NOINLINE void generate_divisor(const random_data<IntT> &vals, IntT denom) {
    for (size_t iter = 0; iter < vals.length(); iter++) {
        (void)generate_1_divisor(denom);
    }
}

struct time_double {
    uint64_t time;  // in nanoseconds
    uint64_t result;
};

enum which_function_t {
    func_hardware,
    func_scalar_branchfull,
    func_scalar_branchfree,
    func_vec_branchfull,
    func_vec_branchfree,
    func_generate
};

template <which_function_t Which, typename IntT>
NOINLINE static time_double time_function(const random_data<IntT> &vals, IntT denom) {
    uint64_t result = 0;
    divider<IntT, BRANCHFULL> div_bfull(denom);
    divider<IntT, BRANCHFREE> div_bfree(denom != 1 ? denom : 2);

    timer t;
    t.start();

    switch (Which) {
        case func_hardware:
            result = sum_quotients(vals, denom);
            break;
        case func_scalar_branchfull:
            result = sum_quotients(vals, div_bfull);
            break;
        case func_scalar_branchfree:
            result = sum_quotients(vals, div_bfree);
            break;
#if defined(x86_VECTOR_TYPE) || defined(LIBDIVIDE_NEON)
        case func_vec_branchfull:
            result = sum_quotients_vec(vals, div_bfull);
            break;
        case func_vec_branchfree:
            result = sum_quotients_vec(vals, div_bfree);
            break;
#endif
        case func_generate:
            generate_divisor(vals, denom);
            break;
        default:
            abort();
    }
    t.stop();

    sGlobalUInt64 += result;

    time_double tresult;
    tresult.time = t.duration_nano();
    tresult.result = result;
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
#define CHECK(actual, expected)              \
    do {                                     \
        if ((actual) != (expected)) {        \
            PRINT_ERROR("Failure on line "); \
            PRINT_ERROR(__LINE__);           \
            PRINT_ERROR("\n");               \
        }                                    \
    } while (0)

template <typename IntT>
NOINLINE TestResult test_one(const random_data<IntT> &vals, IntT denom) {
    const bool testBranchfree = (denom != 1);

    uint64_t min_my_time = INT64_MAX, min_my_time_branchfree = INT64_MAX, min_my_time_vector = INT64_MAX,
        min_my_time_vector_branchfree = INT64_MAX, min_his_time = INT64_MAX, min_gen_time = INT64_MAX;
    time_double tresult;
    for (size_t iter = 0; iter < TEST_COUNT; iter++) {
        tresult = time_function<func_hardware>(vals, denom);
        min_his_time = (std::min)(min_his_time, tresult.time);
        const uint64_t expected = tresult.result;
        tresult = time_function<func_scalar_branchfull>(vals, denom);
        min_my_time = (std::min)(min_my_time, tresult.time);
        CHECK(tresult.result, expected);
        if (testBranchfree) {
            tresult = time_function<func_scalar_branchfree>(vals, denom);
            min_my_time_branchfree = (std::min)(min_my_time_branchfree, tresult.time);
            CHECK(tresult.result, expected);
        }
#if defined(x86_VECTOR_TYPE) || defined(LIBDIVIDE_NEON)
        tresult = time_function<func_vec_branchfull>(vals, denom);
        min_my_time_vector = (std::min)(min_my_time_vector, tresult.time);
        CHECK(tresult.result, expected);
        if (testBranchfree) {
            tresult = time_function<func_vec_branchfree>(vals, denom);
            min_my_time_vector_branchfree = (std::min)(min_my_time_vector_branchfree, tresult.time);
            CHECK(tresult.result, expected);
        }
#else
        min_my_time_vector = 0;
        min_my_time_vector_branchfree = 0;
#endif
        tresult = time_function<func_generate>(vals, denom);
        min_gen_time = (std::min)(min_gen_time, tresult.time);
    }

    TestResult result;
    result.gen_time = min_gen_time / (double)vals.length();
    result.base_time = min_my_time / (double)vals.length();
    result.branchfree_time = testBranchfree ? min_my_time_branchfree / (double)vals.length() : -1;
    result.vector_time = min_my_time_vector / (double)vals.length();
    result.vector_branchfree_time = min_my_time_vector_branchfree / (double)vals.length();
    result.hardware_time = min_his_time / (double)vals.length();
    return result;
}

template <typename _IntT>
int32_t get_algorithm(_IntT d)
{
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
NOINLINE struct TestResult test_one(_IntT d, const random_data<_IntT> &data) {
    struct TestResult result = test_one(data, d);
    result.algo = get_algorithm(d);
    return result;
}

// Result column width
#define PRIcw "10"

static void report_header(void) {
    char buffer[256];
    sprintf(buffer, "%6s %" PRIcw "s %" PRIcw "s %" PRIcw "s %" PRIcw "s %" PRIcw "s %" PRIcw "s %6s\n", "#", "system", "scalar", "scl_bf", "vector", "vec_bf",
        "gener", "algo");
    PRINT_INFO(buffer);
}

// Result column format spec
#define PRIrc PRIcw ".3f"

template <typename _IntT>
static void report_result(_IntT d, struct TestResult result) {
    char denom_buff[32];
    char *pDenom = to_str(denom_buff, d);

    char report_buff[256];
    sprintf(report_buff, "%6s %" PRIrc " %" PRIrc " %" PRIrc " %" PRIrc " %" PRIrc " %" PRIrc " %4d\n", pDenom, result.hardware_time, result.base_time,
        result.branchfree_time, result.vector_time, result.vector_branchfree_time, result.gen_time,
        result.algo);
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
    report_header();
    random_data<_IntT> data;
    _IntT d = 1;
    while (true) {
        report_result(d, test_one(d, data));

        if (std::numeric_limits<_IntT>::is_signed) {
            d = -d;
            if (d > 0) d++;
        } else {
            d++;
        }
    }  
}