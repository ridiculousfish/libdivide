// Usage: benchmark [OPTIONS]
//
// You can pass the benchmark program one or more of the following
// options: u32, s32, u64, s64 to compare libdivide's speed against
// hardware division. If benchmark is run without any options u64
// is used as default option. benchmark tests a simple function that
// inputs an array of random numerators and a single divisor, and
// returns the sum of their quotients. It tests this using both
// hardware division, and the various division approaches supported
// by libdivide, including vector division.

// Silence MSVC sprintf unsafe warnings
#define _CRT_SECURE_NO_WARNINGS

#include "libdivide.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h> 

#if defined(__GNUC__)
    #define NOINLINE __attribute__((__noinline__))
#elif defined(_MSC_VER)
    #define NOINLINE __declspec(noinline)
#else
    #define NOINLINE
#endif

#if defined(LIBDIVIDE_AVX512)
    #define VECTOR_TYPE __m512i
    #define SETZERO_SI _mm512_setzero_si512
    #define LOAD_SI _mm512_load_si512
    #define ADD_EPI64 _mm512_add_epi64
    #define ADD_EPI32 _mm512_add_epi32
#elif defined(LIBDIVIDE_AVX2)
    #define VECTOR_TYPE __m256i
    #define SETZERO_SI _mm256_setzero_si256
    #define LOAD_SI _mm256_load_si256
    #define ADD_EPI64 _mm256_add_epi64
    #define ADD_EPI32 _mm256_add_epi32
#elif defined(LIBDIVIDE_SSE2)
    #define VECTOR_TYPE __m128i
    #define SETZERO_SI _mm_setzero_si128
    #define LOAD_SI _mm_load_si128
    #define ADD_EPI64 _mm_add_epi64
    #define ADD_EPI32 _mm_add_epi32
#endif

#define NANOSEC_PER_SEC 1000000000ULL
#define NANOSEC_PER_USEC 1000ULL
#define NANOSEC_PER_MILLISEC 1000000ULL
#define SEED { 2147483563, 2147483563 ^ 0x49616E42 }

#if defined(__cplusplus)
    using namespace libdivide;
#endif

#if defined(_WIN32) || defined(WIN32)
    #define NOMINMAX
    #define WIN32_LEAN_AND_MEAN
    #define VC_EXTRALEAN
    #include <windows.h>
    #include <mmsystem.h>
    #define LIBDIVIDE_WINDOWS
    #pragma comment(lib, "winmm")
#endif

#if !defined(LIBDIVIDE_WINDOWS)
    #include <sys/time.h> // for gettimeofday()
#endif

struct random_state {
    uint32_t hi;
    uint32_t lo;
};

uint64_t sGlobalUInt64;
size_t iters = 1 << 19;
size_t genIters = 1 << 16;
 
static uint32_t my_random(struct random_state *state) {
    state->hi = (state->hi << 16) + (state->hi >> 16);
    state->hi += state->lo;
    state->lo += state->hi;
    return state->hi;
}
 
#if defined(LIBDIVIDE_WINDOWS)
static LARGE_INTEGER gPerfCounterFreq;
#endif

#if !defined(LIBDIVIDE_WINDOWS)
static uint64_t nanoseconds(void) {
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec * NANOSEC_PER_SEC + now.tv_usec * NANOSEC_PER_USEC;
}
#endif

struct FunctionParams_t {
    const void *d; //a pointer to e.g. a uint32_t
    const void *denomPtr; // a pointer to e.g. libdivide_u32_t
    const void *denomBranchfreePtr; // a pointer to e.g. libdivide_u32_t from branchfree
    const void *data; // a pointer to the data to be divided
};

struct time_result {
    uint64_t time;
    uint64_t result;
};

static struct time_result time_function(uint64_t (*func)(struct FunctionParams_t*), struct FunctionParams_t *params) {
    struct time_result tresult;
#if defined(LIBDIVIDE_WINDOWS)
	LARGE_INTEGER start, end;
	QueryPerformanceCounter(&start);
	uint64_t result = func(params);
	QueryPerformanceCounter(&end);
	uint64_t diff = end.QuadPart - start.QuadPart;
	sGlobalUInt64 += result;
	tresult.result = result;
	tresult.time = (diff * 1000000000) / gPerfCounterFreq.QuadPart;
#else
    uint64_t start = nanoseconds();
    uint64_t result = func(params);
    uint64_t end = nanoseconds();
    uint64_t diff = end - start;
    sGlobalUInt64 += result;
    tresult.result = result;
    tresult.time = diff;
#endif
	return tresult;
}

// U32
 
NOINLINE static uint64_t mine_u32(struct FunctionParams_t *params) {
    const struct libdivide_u32_t denom = *(struct libdivide_u32_t *)params->denomPtr;
    const uint32_t *data = (const uint32_t *)params->data;
    uint32_t sum = 0;
    for (size_t iter = 0; iter < iters; iter++) {
        uint32_t numer = data[iter];
        sum += libdivide_u32_do(numer, &denom);
    }
    return sum;
}

NOINLINE static uint64_t mine_u32_branchfree(struct FunctionParams_t *params) {
    const struct libdivide_u32_branchfree_t denom = *(struct libdivide_u32_branchfree_t *)params->denomBranchfreePtr;
    const uint32_t *data = (const uint32_t *)params->data;
    uint32_t sum = 0;
    for (size_t iter = 0; iter < iters; iter++) {
        uint32_t numer = data[iter];
        sum += libdivide_u32_branchfree_do(numer, &denom);
    }
    return sum;
}

#if defined(LIBDIVIDE_AVX512) || \
    defined(LIBDIVIDE_AVX2) || \
    defined(LIBDIVIDE_SSE2)

NOINLINE static uint64_t mine_u32_vector(struct FunctionParams_t *params) {
    size_t count = sizeof(VECTOR_TYPE) / sizeof(uint32_t);
    const struct libdivide_u32_t denom = *(struct libdivide_u32_t *)params->denomPtr;
    const uint32_t *data = (const uint32_t *)params->data;
    VECTOR_TYPE sumX4 = SETZERO_SI();
        for (size_t iter = 0; iter < iters; iter += count) {
        VECTOR_TYPE numers = LOAD_SI((const VECTOR_TYPE*)(data + iter));
        sumX4 = ADD_EPI32(sumX4, libdivide_u32_do_vector(numers, &denom));
    }
    const uint32_t *comps = (const uint32_t *)&sumX4;
    uint32_t sum = 0;
    for (size_t i = 0; i < count; i++) {
        sum += comps[i];
    }
    return sum;
}

NOINLINE static uint64_t mine_u32_vector_unswitched(struct FunctionParams_t *params) {
    size_t count = sizeof(VECTOR_TYPE) / sizeof(uint32_t);
    const struct libdivide_u32_t denom = *(struct libdivide_u32_t *)params->denomPtr;
    const uint32_t *data = (const uint32_t *)params->data;
    VECTOR_TYPE sumX4 = SETZERO_SI();
    int algo = libdivide_u32_get_algorithm(&denom);
    if (algo == 0) {
        for (size_t iter = 0; iter < iters; iter += count) {
            VECTOR_TYPE numers = LOAD_SI((const VECTOR_TYPE*)(data + iter));
            sumX4 = ADD_EPI32(sumX4, libdivide_u32_do_vector_alg0(numers, &denom));
        }        
    }
    else if (algo == 1) {
        for (size_t iter = 0; iter < iters; iter += count) {
            VECTOR_TYPE numers = LOAD_SI((const VECTOR_TYPE*)(data + iter));
            sumX4 = ADD_EPI32(sumX4, libdivide_u32_do_vector_alg1(numers, &denom));
        }        
    }
    else if (algo == 2) {
        for (size_t iter = 0; iter < iters; iter += count) {
            VECTOR_TYPE numers = LOAD_SI((const VECTOR_TYPE*)(data + iter));
            sumX4 = ADD_EPI32(sumX4, libdivide_u32_do_vector_alg2(numers, &denom));
        }        
    }
    const uint32_t *comps = (const uint32_t *)&sumX4;
    uint32_t sum = 0;
    for (size_t i = 0; i < count; i++) {
        sum += comps[i];
    }
    return sum;
}

NOINLINE static uint64_t mine_u32_vector_branchfree(struct FunctionParams_t *params) {
    size_t count = sizeof(VECTOR_TYPE) / sizeof(uint32_t);
    const struct libdivide_u32_branchfree_t denom = *(struct libdivide_u32_branchfree_t *)params->denomBranchfreePtr;
    const uint32_t *data = (const uint32_t *)params->data;
    VECTOR_TYPE sumX4 = SETZERO_SI();
    for (size_t iter = 0; iter < iters; iter += count) {
        VECTOR_TYPE numers = LOAD_SI((const VECTOR_TYPE*)(data + iter));
        sumX4 = ADD_EPI32(sumX4, libdivide_u32_branchfree_do_vector(numers, &denom));
    }
    const uint32_t *comps = (const uint32_t *)&sumX4;
    uint32_t sum = 0;
    for (size_t i = 0; i < count; i++) {
        sum += comps[i];
    }
    return sum;
    
}

#endif

NOINLINE static uint64_t mine_u32_unswitched(struct FunctionParams_t *params) {
    const struct libdivide_u32_t denom = *(struct libdivide_u32_t *)params->denomPtr;
    const uint32_t *data = (const uint32_t *)params->data;
    uint32_t sum = 0;
    int algo = libdivide_u32_get_algorithm(&denom);
    if (algo == 0) {
        for (size_t iter = 0; iter < iters; iter++) {
            uint32_t numer = data[iter];
            sum += libdivide_u32_do_alg0(numer, &denom);
        }
    }
    else if (algo == 1) {
        for (size_t iter= 0; iter < iters; iter++) {
            uint32_t numer = data[iter];
            sum += libdivide_u32_do_alg1(numer, &denom);
        }        
    }
    else if (algo == 2) {
        for (size_t iter= 0; iter < iters; iter++) {
            uint32_t numer = data[iter];
            sum += libdivide_u32_do_alg2(numer, &denom);
        }        
    }
 
    return sum;
}

NOINLINE static uint64_t his_u32(struct FunctionParams_t *params) {
    const uint32_t *data = (const uint32_t *)params->data;
    const uint32_t d = *(uint32_t *)params->d;
    uint32_t sum = 0;
    for (size_t iter= 0; iter < iters; iter++) {
        uint32_t numer = data[iter];
        sum += numer / d;
    }
    return sum;
}

NOINLINE static uint64_t mine_u32_generate(struct FunctionParams_t *params) {
    uint32_t *dPtr = (uint32_t *)params->d;
    struct libdivide_u32_t *denomPtr = (struct libdivide_u32_t *)params->denomPtr;
    for (size_t iter= 0; iter < genIters; iter++) {
        *denomPtr = libdivide_u32_gen(*dPtr);
    }
    return *dPtr;
}
 
// S32
 
NOINLINE static uint64_t mine_s32(struct FunctionParams_t *params) {
    const struct libdivide_s32_t denom = *(struct libdivide_s32_t *)params->denomPtr;
    const int32_t *data = (const int32_t *)params->data;
    int32_t sum = 0;
    for (size_t iter= 0; iter < iters; iter++) {
        int32_t numer = data[iter];
        sum += libdivide_s32_do(numer, &denom);
    }
    return sum;
}

NOINLINE static uint64_t mine_s32_branchfree(struct FunctionParams_t *params) {
    const struct libdivide_s32_branchfree_t denom = *(struct libdivide_s32_branchfree_t *)params->denomBranchfreePtr;
    const int32_t *data = (const int32_t *)params->data;
    int32_t sum = 0;
    for (size_t iter= 0; iter < iters; iter++) {
        int32_t numer = data[iter];
        sum += libdivide_s32_branchfree_do(numer, &denom);
    }
    return sum;
}

#if defined(LIBDIVIDE_AVX512) || \
    defined(LIBDIVIDE_AVX2) || \
    defined(LIBDIVIDE_SSE2)

NOINLINE static uint64_t mine_s32_vector(struct FunctionParams_t *params) {
    size_t count = sizeof(VECTOR_TYPE) / sizeof(int32_t);
    VECTOR_TYPE sumX4 = SETZERO_SI();
    const struct libdivide_s32_t denom = *(struct libdivide_s32_t *)params->denomPtr;
    const int32_t *data = (const int32_t *)params->data;
    for (size_t iter = 0; iter < iters; iter += count) {
        VECTOR_TYPE numers = LOAD_SI((const VECTOR_TYPE*)(data + iter));
        sumX4 = ADD_EPI32(sumX4, libdivide_s32_do_vector(numers, &denom));
    }
    const int32_t *comps = (const int32_t *)&sumX4;
    int32_t sum = 0;
    for (size_t i = 0; i < count; i++) {
        sum += comps[i];
    }
    return sum;
}

NOINLINE static uint64_t mine_s32_vector_unswitched(struct FunctionParams_t *params) {
    size_t count = sizeof(VECTOR_TYPE) / sizeof(int32_t);
    VECTOR_TYPE sumX4 = SETZERO_SI();
    const struct libdivide_s32_t denom = *(struct libdivide_s32_t *)params->denomPtr;
    const int32_t *data = (const int32_t *)params->data;
    int algo = libdivide_s32_get_algorithm(&denom);
    if (algo == 0) {
        for (size_t iter = 0; iter < iters; iter += count) {
            VECTOR_TYPE numers = LOAD_SI((const VECTOR_TYPE*)(data + iter));
            sumX4 = ADD_EPI32(sumX4, libdivide_s32_do_vector_alg0(numers, &denom));
        }        
    }
    else if (algo == 1) {
        for (size_t iter = 0; iter < iters; iter += count) {
            VECTOR_TYPE numers = LOAD_SI((const VECTOR_TYPE*)(data + iter));
            sumX4 = ADD_EPI32(sumX4, libdivide_s32_do_vector_alg1(numers, &denom));
        }                
    }
    else if (algo == 2) {
        for (size_t iter = 0; iter < iters; iter += count) {
            VECTOR_TYPE numers = LOAD_SI((const VECTOR_TYPE*)(data + iter));
            sumX4 = ADD_EPI32(sumX4, libdivide_s32_do_vector_alg2(numers, &denom));
        }                
    }
    else if (algo == 3) {
        for (size_t iter = 0; iter < iters; iter += count) {
            VECTOR_TYPE numers = LOAD_SI((const VECTOR_TYPE*)(data + iter));
            sumX4 = ADD_EPI32(sumX4, libdivide_s32_do_vector_alg3(numers, &denom));
        }                
    }
    else if (algo == 4) {
        for (size_t iter = 0; iter < iters; iter += count) {
            VECTOR_TYPE numers = LOAD_SI((const VECTOR_TYPE*)(data + iter));
            sumX4 = ADD_EPI32(sumX4, libdivide_s32_do_vector_alg4(numers, &denom));
        }                
    }
    const int32_t *comps = (const int32_t *)&sumX4;
    int32_t sum = 0;
    for (size_t i = 0; i < count; i++) {
        sum += comps[i];
    }
    return sum;
}

NOINLINE static uint64_t mine_s32_vector_branchfree(struct FunctionParams_t *params) {
    size_t count = sizeof(VECTOR_TYPE) / sizeof(int32_t);
    VECTOR_TYPE sumX4 = SETZERO_SI();
    const struct libdivide_s32_branchfree_t denom = *(struct libdivide_s32_branchfree_t *)params->denomBranchfreePtr;
    const int32_t *data = (const int32_t *)params->data;
    for (size_t iter = 0; iter < iters; iter += count) {
        VECTOR_TYPE numers = LOAD_SI((const VECTOR_TYPE*)(data + iter));
        sumX4 = ADD_EPI32(sumX4, libdivide_s32_branchfree_do_vector(numers, &denom));
    }
    const int32_t *comps = (const int32_t *)&sumX4;
    int32_t sum = 0;
    for (size_t i = 0; i < count; i++) {
        sum += comps[i];
    }
    return sum;
}

#endif

NOINLINE static uint64_t mine_s32_unswitched(struct FunctionParams_t *params) {
    int32_t sum = 0;
    const struct libdivide_s32_t denom = *(struct libdivide_s32_t *)params->denomPtr;
    const int32_t *data = (const int32_t *)params->data;
    int algo = libdivide_s32_get_algorithm(&denom);
    if (algo == 0) {
        for (size_t iter= 0; iter < iters; iter++) {
            int32_t numer = data[iter];
            sum += libdivide_s32_do_alg0(numer, &denom);
        }
    }
    else if (algo == 1) {
        for (size_t iter= 0; iter < iters; iter++) {
            int32_t numer = data[iter];
            sum += libdivide_s32_do_alg1(numer, &denom);
        }        
    }
    else if (algo == 2) {
        for (size_t iter= 0; iter < iters; iter++) {
            int32_t numer = data[iter];
            sum += libdivide_s32_do_alg2(numer, &denom);
        }        
    }
    else if (algo == 3) {
        for (size_t iter= 0; iter < iters; iter++) {
            int32_t numer = data[iter];
            sum += libdivide_s32_do_alg3(numer, &denom);
        }        
    }
    else if (algo == 4) {
        for (size_t iter= 0; iter < iters; iter++) {
            int32_t numer = data[iter];
            sum += libdivide_s32_do_alg4(numer, &denom);
        }        
    }
    
    return (uint64_t)sum;
}

NOINLINE static uint64_t his_s32(struct FunctionParams_t *params) {
    int32_t sum = 0;
    const int32_t d = *(int32_t *)params->d;
    const int32_t *data = (const int32_t *)params->data;
    for (size_t iter= 0; iter < iters; iter++) {
        int32_t numer = data[iter];
        sum += numer / d;
    }
    return sum;
}

NOINLINE static uint64_t mine_s32_generate(struct FunctionParams_t *params) {
    int32_t *dPtr = (int32_t *)params->d;
    struct libdivide_s32_t *denomPtr = (struct libdivide_s32_t *)params->denomPtr;
    for (size_t iter= 0; iter < genIters; iter++) {
        *denomPtr = libdivide_s32_gen(*dPtr);
    }
    return *dPtr;
}

// U64

NOINLINE static uint64_t mine_u64(struct FunctionParams_t *params) {
    const struct libdivide_u64_t denom = *(struct libdivide_u64_t *)params->denomPtr;
    const uint64_t *data = (const uint64_t *)params->data;
    uint64_t sum = 0;
    for (size_t iter= 0; iter < iters; iter++) {
        uint64_t numer = data[iter];
        sum += libdivide_u64_do(numer, &denom);
    }
    return sum;
}

NOINLINE static uint64_t mine_u64_branchfree(struct FunctionParams_t *params) {
    const struct libdivide_u64_branchfree_t denom = *(struct libdivide_u64_branchfree_t *)params->denomBranchfreePtr;
    const uint64_t *data = (const uint64_t *)params->data;
    uint64_t sum = 0;
    for (size_t iter= 0; iter < iters; iter++) {
        uint64_t numer = data[iter];
        sum += libdivide_u64_branchfree_do(numer, &denom);
    }
    return sum;
}

NOINLINE static uint64_t mine_u64_unswitched(struct FunctionParams_t *params) {
    uint64_t sum = 0;
    const struct libdivide_u64_t denom = *(struct libdivide_u64_t *)params->denomPtr;
    const uint64_t *data = (const uint64_t *)params->data;
    int algo = libdivide_u64_get_algorithm(&denom);
    if (algo == 0) {
        for (size_t iter= 0; iter < iters; iter++) {
            uint64_t numer = data[iter];
            sum += libdivide_u64_do_alg0(numer, &denom);
        }
    }
    else if (algo == 1) {
        for (size_t iter= 0; iter < iters; iter++) {
            uint64_t numer = data[iter];
            sum += libdivide_u64_do_alg1(numer, &denom);
        }        
    }
    else if (algo == 2) {
        for (size_t iter= 0; iter < iters; iter++) {
            uint64_t numer = data[iter];
            sum += libdivide_u64_do_alg2(numer, &denom);
        }        
    }
    
    return sum;
}

#if defined(LIBDIVIDE_AVX512) || \
    defined(LIBDIVIDE_AVX2) || \
    defined(LIBDIVIDE_SSE2)

NOINLINE static uint64_t mine_u64_vector_unswitched(struct FunctionParams_t *params) {
    size_t count = sizeof(VECTOR_TYPE) / sizeof(uint64_t);
    VECTOR_TYPE sumX2 = SETZERO_SI();
    const struct libdivide_u64_t denom = *(struct libdivide_u64_t *)params->denomPtr;
    const uint64_t *data = (const uint64_t *)params->data;
    int algo = libdivide_u64_get_algorithm(&denom);
    if (algo == 0) {
        for (size_t iter = 0; iter < iters; iter += count) {
            VECTOR_TYPE numers = LOAD_SI((const VECTOR_TYPE*)(data + iter));
            sumX2 = ADD_EPI64(sumX2, libdivide_u64_do_vector_alg0(numers, &denom));
        }   
    }
    else if (algo == 1) {
        for (size_t iter = 0; iter < iters; iter += count) {
            VECTOR_TYPE numers = LOAD_SI((const VECTOR_TYPE*)(data + iter));
            sumX2 = ADD_EPI64(sumX2, libdivide_u64_do_vector_alg1(numers, &denom));
        }   
    }
    else if (algo == 2) {
        for (size_t iter = 0; iter < iters; iter += count) {
            VECTOR_TYPE numers = LOAD_SI((const VECTOR_TYPE*)(data + iter));
            sumX2 = ADD_EPI64(sumX2, libdivide_u64_do_vector_alg2(numers, &denom));
        }        
    }
    const uint64_t *comps = (const uint64_t *)&sumX2;
    uint64_t sum = 0;
    for (size_t i = 0; i < count; i++) {
        sum += comps[i];
    }
    return sum;
}

NOINLINE static uint64_t mine_u64_vector(struct FunctionParams_t *params) {
    size_t count = sizeof(VECTOR_TYPE) / sizeof(uint64_t);
    VECTOR_TYPE sumX2 = SETZERO_SI();
    const struct libdivide_u64_t denom = *(struct libdivide_u64_t *)params->denomPtr;
    const uint64_t *data = (const uint64_t *)params->data;
    for (size_t iter = 0; iter < iters; iter += count) {
        VECTOR_TYPE numers = LOAD_SI((const VECTOR_TYPE*)(data + iter));
        sumX2 = ADD_EPI64(sumX2, libdivide_u64_do_vector(numers, &denom));
    }
    const uint64_t *comps = (const uint64_t *)&sumX2;
    uint64_t sum = 0;
    for (size_t i = 0; i < count; i++) {
        sum += comps[i];
    }
    return sum;
}

NOINLINE static uint64_t mine_u64_vector_branchfree(struct FunctionParams_t *params) {
    size_t count = sizeof(VECTOR_TYPE) / sizeof(uint64_t);
    VECTOR_TYPE sumX2 = SETZERO_SI();
    const struct libdivide_u64_branchfree_t denom = *(struct libdivide_u64_branchfree_t *)params->denomBranchfreePtr;
    const uint64_t *data = (const uint64_t *)params->data;
    for (size_t iter = 0; iter < iters; iter += count) {
        VECTOR_TYPE numers = LOAD_SI((const VECTOR_TYPE*)(data + iter));
        sumX2 = ADD_EPI64(sumX2, libdivide_u64_branchfree_do_vector(numers, &denom));
    }
    const uint64_t *comps = (const uint64_t *)&sumX2;
    uint64_t sum = 0;
    for (size_t i = 0; i < count; i++) {
        sum += comps[i];
    }
    return sum;
}

#endif

NOINLINE static uint64_t his_u64(struct FunctionParams_t *params) {
    uint64_t sum = 0;
    const uint64_t d = *(uint64_t *)params->d;
    const uint64_t *data = (const uint64_t *)params->data;
    for (size_t iter= 0; iter < iters; iter++) {
        uint64_t numer = data[iter];
        sum += numer / d;
    }
    return sum;
}

NOINLINE static uint64_t mine_u64_generate(struct FunctionParams_t *params) {
    uint64_t *dPtr = (uint64_t *)params->d;
    struct libdivide_u64_t *denomPtr = (struct libdivide_u64_t *)params->denomPtr;
    for (size_t iter= 0; iter < genIters; iter++) {
        *denomPtr = libdivide_u64_gen(*dPtr);
    }
    return *dPtr;
}

NOINLINE static uint64_t mine_s64(struct FunctionParams_t *params) {
    const struct libdivide_s64_t denom = *(struct libdivide_s64_t *)params->denomPtr;
    const int64_t *data = (const int64_t *)params->data;
    int64_t sum = 0;
    for (size_t iter= 0; iter < iters; iter++) {
        int64_t numer = data[iter];
        sum += libdivide_s64_do(numer, &denom);
    }
    return sum;
}

NOINLINE static uint64_t mine_s64_branchfree(struct FunctionParams_t *params) {
    const struct libdivide_s64_branchfree_t denom = *(struct libdivide_s64_branchfree_t *)params->denomBranchfreePtr;
    const int64_t *data = (const int64_t *)params->data;
    int64_t sum = 0;
    for (size_t iter= 0; iter < iters; iter++) {
        int64_t numer = data[iter];
        sum += libdivide_s64_branchfree_do(numer, &denom);
    }
    return sum;
}

#if defined(LIBDIVIDE_AVX512) || \
    defined(LIBDIVIDE_AVX2) || \
    defined(LIBDIVIDE_SSE2)

NOINLINE static uint64_t mine_s64_vector(struct FunctionParams_t *params) {
    const struct libdivide_s64_t denom = *(struct libdivide_s64_t *)params->denomPtr;
    const int64_t *data = (const int64_t *)params->data;
    size_t count = sizeof(VECTOR_TYPE) / sizeof(int64_t);
    
    VECTOR_TYPE sumX2 = SETZERO_SI();
    for (size_t iter = 0; iter < iters; iter += count) {
        VECTOR_TYPE numers = LOAD_SI((const VECTOR_TYPE*)(data + iter));
        sumX2 = ADD_EPI64(sumX2, libdivide_s64_do_vector(numers, &denom));
    }
    const int64_t *comps = (const int64_t *)&sumX2;
    int64_t sum = 0;
    for (size_t i = 0; i < count; i++) {
        sum += comps[i];
    }
    return sum;
}

NOINLINE static uint64_t mine_s64_vector_branchfree(struct FunctionParams_t *params) {
    const struct libdivide_s64_branchfree_t denom = *(struct libdivide_s64_branchfree_t *)params->denomBranchfreePtr;
    const int64_t *data = (const int64_t *)params->data;
    size_t count = sizeof(VECTOR_TYPE) / sizeof(int64_t);
    
    VECTOR_TYPE sumX2 = SETZERO_SI();
    for (size_t iter = 0; iter < iters; iter += count) {
        VECTOR_TYPE numers = LOAD_SI((const VECTOR_TYPE*)(data + iter));
        sumX2 = ADD_EPI64(sumX2, libdivide_s64_branchfree_do_vector(numers, &denom));
    }
    const int64_t *comps = (const int64_t *)&sumX2;
    int64_t sum = 0;
    for (size_t i = 0; i < count; i++) {
        sum += comps[i];
    }
    return sum;
}

NOINLINE static uint64_t mine_s64_vector_unswitched(struct FunctionParams_t *params) {
    const struct libdivide_s64_t denom = *(struct libdivide_s64_t *)params->denomPtr;
    const int64_t *data = (const int64_t *)params->data;
    size_t count = sizeof(VECTOR_TYPE) / sizeof(int64_t);
    
    VECTOR_TYPE sumX2 = SETZERO_SI();
    int algo = libdivide_s64_get_algorithm(&denom);
    if (algo == 0) {
        for (size_t iter = 0; iter < iters; iter += count) {
            VECTOR_TYPE numers = LOAD_SI((const VECTOR_TYPE*)(data + iter));
            sumX2 = ADD_EPI64(sumX2, libdivide_s64_do_vector_alg0(numers, &denom));
        }        
    }
    else if (algo == 1) {
        for (size_t iter = 0; iter < iters; iter += count) {
            VECTOR_TYPE numers = LOAD_SI((const VECTOR_TYPE*)(data + iter));
            sumX2 = ADD_EPI64(sumX2, libdivide_s64_do_vector_alg1(numers, &denom));
        }        
    }
    else if (algo == 2) {
        for (size_t iter = 0; iter < iters; iter += count) {
            VECTOR_TYPE numers = LOAD_SI((const VECTOR_TYPE*)(data + iter));
            sumX2 = ADD_EPI64(sumX2, libdivide_s64_do_vector_alg2(numers, &denom));
        }        
    }
    else if (algo == 3) {
        for (size_t iter = 0; iter < iters; iter += count) {
            VECTOR_TYPE numers = LOAD_SI((const VECTOR_TYPE*)(data + iter));
            sumX2 = ADD_EPI64(sumX2, libdivide_s64_do_vector_alg3(numers, &denom));
        }        
    }
    else if (algo == 4) {
        for (size_t iter = 0; iter < iters; iter += count) {
            VECTOR_TYPE numers = LOAD_SI((const VECTOR_TYPE*)(data + iter));
            sumX2 = ADD_EPI64(sumX2, libdivide_s64_do_vector_alg4(numers, &denom));
        }        
    }
    const int64_t *comps = (const int64_t *)&sumX2;
    int64_t sum = 0;
    for (size_t i = 0; i < count; i++) {
        sum += comps[i];
    }
    return sum;
}

#endif

NOINLINE static uint64_t mine_s64_unswitched(struct FunctionParams_t *params) {
    const struct libdivide_s64_t denom = *(struct libdivide_s64_t *)params->denomPtr;
    const int64_t *data = (const int64_t *)params->data;
    int64_t sum = 0;
    int algo = libdivide_s64_get_algorithm(&denom);

    if (algo == 0) {
        for (size_t iter= 0; iter < iters; iter++) {
            int64_t numer = data[iter];
            sum += libdivide_s64_do_alg0(numer, &denom);
        }
    }
    else if (algo == 1) {
        for (size_t iter= 0; iter < iters; iter++) {
            int64_t numer = data[iter];
            sum += libdivide_s64_do_alg1(numer, &denom);
        }        
    }
    else if (algo == 2) {
        for (size_t iter= 0; iter < iters; iter++) {
            int64_t numer = data[iter];
            sum += libdivide_s64_do_alg2(numer, &denom);
        }        
    }
    else if (algo == 3) {
        for (size_t iter= 0; iter < iters; iter++) {
            int64_t numer = data[iter];
            sum += libdivide_s64_do_alg3(numer, &denom);
        }        
    }
    else if (algo == 4) {
        for (size_t iter= 0; iter < iters; iter++) {
            int64_t numer = data[iter];
            sum += libdivide_s64_do_alg4(numer, &denom);
        }        
    }
    
    return sum;
}

NOINLINE static uint64_t his_s64(struct FunctionParams_t *params) {
    const int64_t *data = (const int64_t *)params->data;
    const int64_t d = *(int64_t *)params->d;
    int64_t sum = 0;
    for (size_t iter= 0; iter < iters; iter++) {
        int64_t numer = data[iter];
        sum += numer / d;
    }
    return sum;
}

NOINLINE static uint64_t mine_s64_generate(struct FunctionParams_t *params) {
    int64_t *dPtr = (int64_t *)params->d;
    struct libdivide_s64_t *denomPtr = (struct libdivide_s64_t *)params->denomPtr;
    for (size_t iter= 0; iter < genIters; iter++) {
        *denomPtr = libdivide_s64_gen(*dPtr);
    }
    return *dPtr;
}

/* Stub functions for when we have no AVX512/AVX2/SSE2 */
#if !defined(LIBDIVIDE_AVX512) && \
    !defined(LIBDIVIDE_AVX2) && \
    !defined(LIBDIVIDE_SSE2)
    
NOINLINE static uint64_t mine_u32_vector(struct FunctionParams_t *params) { return mine_u32(params); }
NOINLINE static uint64_t mine_u32_vector_unswitched(struct FunctionParams_t *params) { return mine_u32_unswitched(params); }
NOINLINE static uint64_t mine_u32_vector_branchfree(struct FunctionParams_t *params) { return mine_u32_branchfree(params); }
NOINLINE static uint64_t mine_s32_vector(struct FunctionParams_t *params) { return mine_s32(params); }
NOINLINE static uint64_t mine_s32_vector_unswitched(struct FunctionParams_t *params) { return mine_s32_unswitched(params); }
NOINLINE static uint64_t mine_s32_vector_branchfree(struct FunctionParams_t *params) { return mine_s32_branchfree(params); }
NOINLINE static uint64_t mine_u64_vector(struct FunctionParams_t *params) { return mine_u64(params); }
NOINLINE static uint64_t mine_u64_vector_unswitched(struct FunctionParams_t *params) { return mine_u64_unswitched(params); }
NOINLINE static uint64_t mine_u64_vector_branchfree(struct FunctionParams_t *params) { return mine_u64_branchfree(params); }
NOINLINE static uint64_t mine_s64_vector(struct FunctionParams_t *params) { return mine_s64(params); }
NOINLINE static uint64_t mine_s64_vector_unswitched(struct FunctionParams_t *params) { return mine_s64_unswitched(params); }
NOINLINE static uint64_t mine_s64_vector_branchfree(struct FunctionParams_t *params) { return mine_s64_branchfree(params); }

#endif
 
struct TestResult {
    double my_base_time;
    double my_branchfree_time;
    double my_unswitched_time;
    double my_vector_time;
    double my_vector_branchfree_time;
    double my_vector_unswitched_time;
    double his_time;
    double gen_time;
    int algo;
};
 
static uint64_t find_min(const uint64_t *vals, size_t cnt) {
    uint64_t result = vals[0];
    size_t i;
    for (i=1; i < cnt; i++) {
        if (vals[i] < result) result = vals[i];
    }
    return result;
}
 
typedef uint64_t (*TestFunc_t)(struct FunctionParams_t *params);
 
NOINLINE struct TestResult test_one(TestFunc_t mine, TestFunc_t mine_branchfree, TestFunc_t mine_vector, TestFunc_t mine_unswitched, TestFunc_t mine_vector_unswitched, TestFunc_t mine_vector_branchfree, TestFunc_t his, TestFunc_t generate, struct FunctionParams_t *params) {
#define TEST_COUNT 30
    struct TestResult result;
    memset(&result, 0, sizeof result);
    
#define CHECK(actual, expected) do { if (1 && actual != expected) printf("Failure on line %lu\n", (unsigned long)__LINE__); } while (0)
    
    uint64_t my_times[TEST_COUNT], my_times_branchfree[TEST_COUNT], my_times_unswitched[TEST_COUNT], my_times_vector[TEST_COUNT], my_times_vector_unswitched[TEST_COUNT], my_times_vector_branchfree[TEST_COUNT], his_times[TEST_COUNT], gen_times[TEST_COUNT];
    struct time_result tresult;
    for (size_t iter= 0; iter < TEST_COUNT; iter++) {
        tresult = time_function(his, params); his_times[iter] = tresult.time; const uint64_t expected = tresult.result;
        tresult = time_function(mine, params); my_times[iter] = tresult.time; CHECK(tresult.result, expected);
        tresult = time_function(mine_branchfree, params); my_times_branchfree[iter] = tresult.time; CHECK(tresult.result, expected);
        tresult = time_function(mine_unswitched, params); my_times_unswitched[iter] = tresult.time; CHECK(tresult.result, expected);
#if defined(LIBDIVIDE_AVX512) || \
    defined(LIBDIVIDE_AVX2) || \
    defined(LIBDIVIDE_SSE2)
        tresult = time_function(mine_vector, params); my_times_vector[iter] = tresult.time; CHECK(tresult.result, expected);
        tresult = time_function(mine_vector_branchfree, params); my_times_vector_branchfree[iter] = tresult.time; CHECK(tresult.result, expected);
        tresult = time_function(mine_vector_unswitched, params); my_times_vector_unswitched[iter] = tresult.time; CHECK(tresult.result, expected);
#else
		my_times_vector[iter]=0;
		my_times_vector_unswitched[iter] = 0;
        my_times_vector_branchfree[iter] = 0;
        (void) mine_vector;
        (void) mine_vector_unswitched;
        (void) mine_vector_branchfree;
#endif
        tresult = time_function(generate, params); gen_times[iter] = tresult.time;
    }
        
    result.gen_time = find_min(gen_times, TEST_COUNT) / (double)genIters;
    result.my_base_time = find_min(my_times, TEST_COUNT) / (double)iters;
    result.my_branchfree_time = find_min(my_times_branchfree, TEST_COUNT) / (double)iters;
    result.my_vector_time = find_min(my_times_vector, TEST_COUNT) / (double)iters;
	//printf("%f - %f\n", find_min(my_times_vector, TEST_COUNT) / (double)iters, result.my_vector_time);
    result.my_unswitched_time = find_min(my_times_unswitched, TEST_COUNT) / (double)iters;
    result.my_vector_branchfree_time = find_min(my_times_vector_branchfree, TEST_COUNT) / (double)iters;
    result.my_vector_unswitched_time = find_min(my_times_vector_unswitched, TEST_COUNT) / (double)iters;
    result.his_time = find_min(his_times, TEST_COUNT) / (double)iters;
    return result;
#undef TEST_COUNT
}
 
NOINLINE struct TestResult test_one_u32(uint32_t d, const uint32_t *data) {
    int no_branchfree = (d == 1);
    struct libdivide_u32_t div_struct = libdivide_u32_gen(d);
    struct libdivide_u32_branchfree_t div_struct_bf = libdivide_u32_branchfree_gen(no_branchfree ? 2 : d);
    struct FunctionParams_t params;
    params.d = &d;
    params.denomPtr = &div_struct;
    params.denomBranchfreePtr = &div_struct_bf;
    params.data = data;
        
    struct TestResult result = test_one(mine_u32,
                                        no_branchfree ? mine_u32 : mine_u32_branchfree,
                                        mine_u32_vector,
                                        mine_u32_unswitched,
                                        mine_u32_vector_unswitched,
                                        no_branchfree ? mine_u32_vector : mine_u32_vector_branchfree,
                                        his_u32,
                                        mine_u32_generate,
                                        &params);
    result.algo = libdivide_u32_get_algorithm(&div_struct);
    return result;
}
 
NOINLINE struct TestResult test_one_s32(int32_t d, const int32_t *data) {
    int no_branchfree = (d == 1 || d == -1);
    struct libdivide_s32_t div_struct = libdivide_s32_gen(d);
    struct libdivide_s32_branchfree_t div_struct_bf = libdivide_s32_branchfree_gen(no_branchfree ? 2 : d);
    struct FunctionParams_t params;
    params.d = &d;
    params.denomPtr = &div_struct;
    params.denomBranchfreePtr = &div_struct_bf;
    params.data = data;
    
    struct TestResult result = test_one(mine_s32,
                                        no_branchfree ? mine_s32 : mine_s32_branchfree,
                                        mine_s32_vector,
                                        mine_s32_unswitched,
                                        mine_s32_vector_unswitched,
                                        no_branchfree ? mine_s32_vector : mine_s32_vector_branchfree,
                                        his_s32,
                                        mine_s32_generate,
                                        &params);
    result.algo = libdivide_s32_get_algorithm(&div_struct);
    return result;
}
 
NOINLINE struct TestResult test_one_u64(uint64_t d, const uint64_t *data) {
    int no_branchfree = (d == 1);
    struct libdivide_u64_t div_struct = libdivide_u64_gen(d);
    struct libdivide_u64_branchfree_t div_struct_bf = libdivide_u64_branchfree_gen(no_branchfree ? 2 : d);
    struct FunctionParams_t params;
    params.d = &d;
    params.denomPtr = &div_struct;
    params.denomBranchfreePtr = &div_struct_bf;
    params.data = data;
    
    struct TestResult result = test_one(mine_u64,
                                        no_branchfree ? mine_u64 : mine_u64_branchfree,
                                        mine_u64_vector,
                                        mine_u64_unswitched,
                                        mine_u64_vector_unswitched,
                                        no_branchfree ? mine_u64_vector : mine_u64_vector_branchfree,
                                        his_u64,
                                        mine_u64_generate,
                                        &params);
    result.algo = libdivide_u64_get_algorithm(&div_struct);
    return result;
}
 
NOINLINE struct TestResult test_one_s64(int64_t d, const int64_t *data) {
    int no_branchfree = (d == 1 || d == -1);
    struct libdivide_s64_t div_struct = libdivide_s64_gen(d);
    struct libdivide_s64_branchfree_t div_struct_bf = libdivide_s64_branchfree_gen(no_branchfree ? 2 : d);
    struct FunctionParams_t params;
    params.d = &d;
    params.denomPtr = &div_struct;
    params.denomBranchfreePtr = no_branchfree ? (void *)&div_struct : (void *)&div_struct_bf;
    params.data = data;
    
    struct TestResult result = test_one(mine_s64, no_branchfree ? mine_s64 : mine_s64_branchfree, mine_s64_vector, mine_s64_unswitched, mine_s64_vector_unswitched, mine_s64_vector_branchfree, his_s64, mine_s64_generate, &params);
    result.algo = libdivide_s64_get_algorithm(&div_struct);
    return result;
}

static void report_header(void) {
    printf("%6s%8s%8s%8s%8s%8s%8s%8s%8s%6s\n", "#", "system", "scalar", "scl_bf", "scl_us", "vector", "vec_bf", "vec_us", "gener", "algo");
}

static void report_result(const char *input, struct TestResult result) {
    printf("%6s%8.3f%8.3f%8.3f%8.3f%8.3f%8.3f%8.3f%8.3f%6d\n", input, result.his_time, result.my_base_time, result.my_branchfree_time, result.my_unswitched_time, result.my_vector_time, result.my_vector_branchfree_time, result.my_vector_unswitched_time, result.gen_time, result.algo);
}

static void test_many_u32(const uint32_t *data) {
    report_header();
    uint32_t d;
    for (d=1; d > 0; d++) {
        struct TestResult result = test_one_u32(d, data);
        char input_buff[32];
        sprintf(input_buff, "%u", d);
        report_result(input_buff, result);
    }
}

static void test_many_s32(const int32_t *data) {
    report_header();
    int32_t d;
    for (d=1; d != 0;) {
        struct TestResult result = test_one_s32(d, data);
        char input_buff[32];
        sprintf(input_buff, "%d", d);
        report_result(input_buff, result);
        
        d = -d;
        if (d > 0) d++;
    }
}

static void test_many_u64(const uint64_t *data) {
    report_header();
    uint64_t d;
    for (d=1; d > 0; d++) {
        struct TestResult result = test_one_u64(d, data);
        char input_buff[32];
        sprintf(input_buff, "%" PRIu64, d);
        report_result(input_buff, result);
    }    
}

static void test_many_s64(const int64_t *data) {
    report_header();
    int64_t d;
    for (d=1; d != 0;) {
        struct TestResult result = test_one_s64(d, data);
        char input_buff[32];
        sprintf(input_buff, "%" PRId64, d);
        report_result(input_buff, result);
        
        d = -d;
        if (d > 0) d++;
    }
}

static const uint32_t *random_data(unsigned sizeOfType) {
#if defined(LIBDIVIDE_WINDOWS)
    /* Align memory to 64 byte boundary for AVX512 */
    uint32_t *data = (uint32_t *) _aligned_malloc(iters * sizeOfType, 64);
#else
    /* Align memory to 64 byte boundary for AVX512 */
    void *ptr = NULL;
    int failed = posix_memalign(&ptr, 64, iters * sizeOfType);
    if (failed) {
        printf("Failed to align memory!\n");
        exit(1);
    }
    uint32_t *data = (uint32_t*) ptr;
#endif
    size_t size = (iters * sizeOfType) / sizeof(*data);
    struct random_state state = SEED;
    for (size_t i = 0; i < size; i++) {
        data[i] = my_random(&state);
    }
    return data;
}

int main(int argc, char* argv[]) {
#if defined(LIBDIVIDE_WINDOWS)
	QueryPerformanceFrequency(&gPerfCounterFreq);
#endif
    int u32 = 0;
    int s32 = 0;
    int u64 = 0;
    int s64 = 0;

    if (argc == 1) {
        // By default test only u64
        u64 = 1;
    }
    else {
        for (int i = 1; i < argc; i++) {
            if (! strcmp(argv[i], "u32")) u32 = 1;
            else if (! strcmp(argv[i], "u64")) u64 = 1;
            else if (! strcmp(argv[i], "s32")) s32 = 1;
            else if (! strcmp(argv[i], "s64")) s64 = 1;
            else {
                printf("Usage: benchmark [OPTIONS]\n"
                       "\n"
                       "You can pass the benchmark program one or more of the following\n"
                       "options: u32, s32, u64, s64 to compare libdivide's speed against\n"
                       "hardware division. If benchmark is run without any options u64\n"
                       "is used as default option. benchmark tests a simple function that\n"
                       "inputs an array of random numerators and a single divisor, and\n"
                       "returns the sum of their quotients. It tests this using both\n"
                       "hardware division, and the various division approaches supported\n"
                       "by libdivide, including vector division.\n");
                exit(1);
            }
        }
    }

    // Make sure that the number of iterations is not
    // known at compile time to prevent the compiler
    // from magically calculating results at compile
    // time and hence falsifying the benchmark.
    srand((unsigned) time(NULL));
    iters += (rand() % 3) * (1 << 10);
    genIters += (rand() % 3) * (1 << 10);

    const uint32_t *data = NULL;
    data = random_data(sizeof(uint32_t));
    if (u32) test_many_u32(data);
    if (s32) test_many_s32((const int32_t *)data);

#if defined(LIBDIVIDE_WINDOWS)
    _aligned_free((void *)data);
#else
    free((void *)data);
#endif

    data = random_data(sizeof(uint64_t));
    if (u64) test_many_u64((const uint64_t *)data);
    if (s64) test_many_s64((const int64_t *)data);

#if defined(LIBDIVIDE_WINDOWS)
    _aligned_free((void *)data);
#else
    free((void *)data);
#endif

    return 0;
}
