// Silence MSVC sprintf unsafe warnings
#define _CRT_SECURE_NO_WARNINGS

#include "libdivide.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#if defined(__GNUC__)
#define NOINLINE __attribute__((__noinline__))
#else
#define NOINLINE
#endif
 
#define NANOSEC_PER_SEC 1000000000ULL
#define NANOSEC_PER_USEC 1000ULL
#define NANOSEC_PER_MILLISEC 1000000ULL

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
 
#define SEED {2147483563, 2147483563 ^ 0x49616E42}
#define ITERATIONS (1 << 19)
#define GEN_ITERATIONS (1 << 16)
 
uint64_t sGlobalUInt64;
 
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

//U32
 
NOINLINE
static uint64_t mine_u32(struct FunctionParams_t *params) {
    unsigned iter;
    const struct libdivide_u32_t denom = *(struct libdivide_u32_t *)params->denomPtr;
    const uint32_t *data = (const uint32_t *)params->data;
    uint32_t sum = 0;
    for (iter = 0; iter < ITERATIONS; iter++) {
        uint32_t numer = data[iter];
        sum += libdivide_u32_do(numer, &denom);
    }
    return sum;
}

NOINLINE
static uint64_t mine_u32_branchfree(struct FunctionParams_t *params) {
    unsigned iter;
    const struct libdivide_u32_branchfree_t denom = *(struct libdivide_u32_branchfree_t *)params->denomBranchfreePtr;
    const uint32_t *data = (const uint32_t *)params->data;
    uint32_t sum = 0;
    for (iter = 0; iter < ITERATIONS; iter++) {
        uint32_t numer = data[iter];
        sum += libdivide_u32_branchfree_do(numer, &denom);
    }
    return sum;
}


#if defined(LIBDIVIDE_USE_SSE2)
NOINLINE static uint64_t mine_u32_vector(struct FunctionParams_t *params) {
    unsigned iter;
    const struct libdivide_u32_t denom = *(struct libdivide_u32_t *)params->denomPtr;
    const uint32_t *data = (const uint32_t *)params->data;
    __m128i sumX4 = _mm_setzero_si128();
    for (iter = 0; iter < ITERATIONS; iter+=4) {
        __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
        sumX4 = _mm_add_epi32(sumX4, libdivide_u32_do_vector(numers, &denom));
    }
    const uint32_t *comps = (const uint32_t *)&sumX4;
    return comps[0] + comps[1] + comps[2] + comps[3];
}

NOINLINE static uint64_t mine_u32_vector_unswitched(struct FunctionParams_t *params) {
    unsigned iter;
    const struct libdivide_u32_t denom = *(struct libdivide_u32_t *)params->denomPtr;
    const uint32_t *data = (const uint32_t *)params->data;
    __m128i sumX4 = _mm_setzero_si128();
    int algo = libdivide_u32_get_algorithm(&denom);
    if (algo == 0) {
        for (iter = 0; iter < ITERATIONS; iter+=4) {
            __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
            sumX4 = _mm_add_epi32(sumX4, libdivide_u32_do_vector_alg0(numers, &denom));
        }        
    }
    else if (algo == 1) {
        for (iter = 0; iter < ITERATIONS; iter+=4) {
            __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
            sumX4 = _mm_add_epi32(sumX4, libdivide_u32_do_vector_alg1(numers, &denom));
        }        
    }
    else if (algo == 2) {
        for (iter = 0; iter < ITERATIONS; iter+=4) {
            __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
            sumX4 = _mm_add_epi32(sumX4, libdivide_u32_do_vector_alg2(numers, &denom));
        }        
    }
    const uint32_t *comps = (const uint32_t *)&sumX4;
    return comps[0] + comps[1] + comps[2] + comps[3];
}


NOINLINE static uint64_t mine_u32_vector_branchfree(struct FunctionParams_t *params) {
    unsigned iter;
    const struct libdivide_u32_branchfree_t denom = *(struct libdivide_u32_branchfree_t *)params->denomBranchfreePtr;
    const uint32_t *data = (const uint32_t *)params->data;
    __m128i sumX4 = _mm_setzero_si128();
    for (iter = 0; iter < ITERATIONS; iter+=4) {
        __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
        sumX4 = _mm_add_epi32(sumX4, libdivide_u32_branchfree_do_vector(numers, &denom));
    }
    const uint32_t *comps = (const uint32_t *)&sumX4;
    return comps[0] + comps[1] + comps[2] + comps[3];
    
}
#endif

#if defined(LIBDIVIDE_USE_AVX2)
NOINLINE static uint64_t mine_u32_vector(struct FunctionParams_t *params) {
    unsigned iter;
    const struct libdivide_u32_t denom = *(struct libdivide_u32_t *)params->denomPtr;
    const uint32_t *data = (const uint32_t *)params->data;
    __m256i sumX4 = _mm256_setzero_si256();
    for (iter = 0; iter < ITERATIONS; iter+=8) {
        __m256i numers = _mm256_load_si256((const __m256i*)(data + iter));
        sumX4 = _mm256_add_epi32(sumX4, libdivide_u32_do_vector(numers, &denom));
    }
    const uint32_t *comps = (const uint32_t *)&sumX4;
    return comps[0] + comps[1] + comps[2] + comps[3];
}

NOINLINE static uint64_t mine_u32_vector_unswitched(struct FunctionParams_t *params) {
    unsigned iter;
    const struct libdivide_u32_t denom = *(struct libdivide_u32_t *)params->denomPtr;
    const uint32_t *data = (const uint32_t *)params->data;
    __m256i sumX4 = _mm256_setzero_si256();
    int algo = libdivide_u32_get_algorithm(&denom);
    if (algo == 0) {
        for (iter = 0; iter < ITERATIONS; iter+=8) {
            __m256i numers = _mm256_load_si256((const __m256i*)(data + iter));
            sumX4 = _mm256_add_epi32(sumX4, libdivide_u32_do_vector_alg0(numers, &denom));
        }        
    }
    else if (algo == 1) {
        for (iter = 0; iter < ITERATIONS; iter+=8) {
            __m256i numers = _mm256_load_si256((const __m256i*)(data + iter));
            sumX4 = _mm256_add_epi32(sumX4, libdivide_u32_do_vector_alg1(numers, &denom));
        }        
    }
    else if (algo == 2) {
        for (iter = 0; iter < ITERATIONS; iter+=8) {
            __m256i numers = _mm256_load_si256((const __m256i*)(data + iter));
            sumX4 = _mm256_add_epi32(sumX4, libdivide_u32_do_vector_alg2(numers, &denom));
        }        
    }
    const uint32_t *comps = (const uint32_t *)&sumX4;
    return comps[0] + comps[1] + comps[2] + comps[3];
}


NOINLINE static uint64_t mine_u32_vector_branchfree(struct FunctionParams_t *params) {
    unsigned iter;
    const struct libdivide_u32_branchfree_t denom = *(struct libdivide_u32_branchfree_t *)params->denomBranchfreePtr;
    const uint32_t *data = (const uint32_t *)params->data;
    __m256i sumX4 = _mm256_setzero_si256();
    for (iter = 0; iter < ITERATIONS; iter+=8) {
        __m256i numers = _mm256_load_si256((const __m256i*)(data + iter));
        sumX4 = _mm256_add_epi32(sumX4, libdivide_u32_branchfree_do_vector(numers, &denom));
    }
    const uint32_t *comps = (const uint32_t *)&sumX4;
    return comps[0] + comps[1] + comps[2] + comps[3];
    
}
#endif

NOINLINE
static uint64_t mine_u32_unswitched(struct FunctionParams_t *params) {
    unsigned iter;
    const struct libdivide_u32_t denom = *(struct libdivide_u32_t *)params->denomPtr;
    const uint32_t *data = (const uint32_t *)params->data;
    uint32_t sum = 0;
    int algo = libdivide_u32_get_algorithm(&denom);
    if (algo == 0) {
        for (iter = 0; iter < ITERATIONS; iter++) {
            uint32_t numer = data[iter];
            sum += libdivide_u32_do_alg0(numer, &denom);
        }
    }
    else if (algo == 1) {
        for (iter = 0; iter < ITERATIONS; iter++) {
            uint32_t numer = data[iter];
            sum += libdivide_u32_do_alg1(numer, &denom);
        }        
    }
    else if (algo == 2) {
        for (iter = 0; iter < ITERATIONS; iter++) {
            uint32_t numer = data[iter];
            sum += libdivide_u32_do_alg2(numer, &denom);
        }        
    }
 
    return sum;
}
 
NOINLINE
static uint64_t his_u32(struct FunctionParams_t *params) {
    unsigned iter;
    const uint32_t *data = (const uint32_t *)params->data;
    const uint32_t d = *(uint32_t *)params->d;
    uint32_t sum = 0;
    for (iter = 0; iter < ITERATIONS; iter++) {
        uint32_t numer = data[iter];
        sum += numer / d;
    }
    return sum;
}
 
NOINLINE
static uint64_t mine_u32_generate(struct FunctionParams_t *params) {
    uint32_t *dPtr = (uint32_t *)params->d;
    struct libdivide_u32_t *denomPtr = (struct libdivide_u32_t *)params->denomPtr;
    unsigned iter;
    for (iter = 0; iter < GEN_ITERATIONS; iter++) {
        *denomPtr = libdivide_u32_gen(*dPtr);
    }
    return *dPtr;
}
 
//S32
 
NOINLINE
static uint64_t mine_s32(struct FunctionParams_t *params) {
    unsigned iter;
    const struct libdivide_s32_t denom = *(struct libdivide_s32_t *)params->denomPtr;
    const int32_t *data = (const int32_t *)params->data;
    int32_t sum = 0;
    for (iter = 0; iter < ITERATIONS; iter++) {
        int32_t numer = data[iter];
        sum += libdivide_s32_do(numer, &denom);
    }
    return sum;
}

NOINLINE
static uint64_t mine_s32_branchfree(struct FunctionParams_t *params) {
    unsigned iter;
    const struct libdivide_s32_branchfree_t denom = *(struct libdivide_s32_branchfree_t *)params->denomBranchfreePtr;
    const int32_t *data = (const int32_t *)params->data;
    int32_t sum = 0;
    for (iter = 0; iter < ITERATIONS; iter++) {
        int32_t numer = data[iter];
        sum += libdivide_s32_branchfree_do(numer, &denom);
    }
    return sum;
}

#if defined(LIBDIVIDE_USE_SSE2)
NOINLINE
static uint64_t mine_s32_vector(struct FunctionParams_t *params) {
    unsigned iter;
    __m128i sumX4 = _mm_setzero_si128();
    const struct libdivide_s32_t denom = *(struct libdivide_s32_t *)params->denomPtr;
    const int32_t *data = (const int32_t *)params->data;
    for (iter = 0; iter < ITERATIONS; iter+=4) {
        __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
        sumX4 = _mm_add_epi32(sumX4, libdivide_s32_do_vector(numers, &denom));
    }
    const int32_t *comps = (const int32_t *)&sumX4;
    int32_t sum = comps[0] + comps[1] + comps[2] + comps[3];
    return sum;
}

NOINLINE
static uint64_t mine_s32_vector_unswitched(struct FunctionParams_t *params) {
    unsigned iter;
    __m128i sumX4 = _mm_setzero_si128();
    const struct libdivide_s32_t denom = *(struct libdivide_s32_t *)params->denomPtr;
    const int32_t *data = (const int32_t *)params->data;
    int algo = libdivide_s32_get_algorithm(&denom);
    if (algo == 0) {
        for (iter = 0; iter < ITERATIONS; iter+=4) {
            __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
            sumX4 = _mm_add_epi32(sumX4, libdivide_s32_do_vector_alg0(numers, &denom));
        }        
    }
    else if (algo == 1) {
        for (iter = 0; iter < ITERATIONS; iter+=4) {
            __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
            sumX4 = _mm_add_epi32(sumX4, libdivide_s32_do_vector_alg1(numers, &denom));
        }                
    }
    else if (algo == 2) {
        for (iter = 0; iter < ITERATIONS; iter+=4) {
            __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
            sumX4 = _mm_add_epi32(sumX4, libdivide_s32_do_vector_alg2(numers, &denom));
        }                
    }
    else if (algo == 3) {
        for (iter = 0; iter < ITERATIONS; iter+=4) {
            __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
            sumX4 = _mm_add_epi32(sumX4, libdivide_s32_do_vector_alg3(numers, &denom));
        }                
    }
    else if (algo == 4) {
        for (iter = 0; iter < ITERATIONS; iter+=4) {
            __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
            sumX4 = _mm_add_epi32(sumX4, libdivide_s32_do_vector_alg4(numers, &denom));
        }                
    }
    const int32_t *comps = (const int32_t *)&sumX4;
    int32_t sum = comps[0] + comps[1] + comps[2] + comps[3];
    return sum;
}

NOINLINE
static uint64_t mine_s32_vector_branchfree(struct FunctionParams_t *params) {
    unsigned iter;
    __m128i sumX4 = _mm_setzero_si128();
    const struct libdivide_s32_branchfree_t denom = *(struct libdivide_s32_branchfree_t *)params->denomBranchfreePtr;
    const int32_t *data = (const int32_t *)params->data;
    for (iter = 0; iter < ITERATIONS; iter+=4) {
        __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
        sumX4 = _mm_add_epi32(sumX4, libdivide_s32_branchfree_do_vector(numers, &denom));
    }
    const int32_t *comps = (const int32_t *)&sumX4;
    int32_t sum = comps[0] + comps[1] + comps[2] + comps[3];
    return sum;
}

#endif

#if defined(LIBDIVIDE_USE_AVX2)
NOINLINE
static uint64_t mine_s32_vector(struct FunctionParams_t *params) {
    unsigned iter;
    __m256i sumX4 = _mm256_setzero_si256();
    const struct libdivide_s32_t denom = *(struct libdivide_s32_t *)params->denomPtr;
    const int32_t *data = (const int32_t *)params->data;
    for (iter = 0; iter < ITERATIONS; iter+=8) {
        __m256i numers = _mm256_load_si256((const __m256i*)(data + iter));
        sumX4 = _mm256_add_epi32(sumX4, libdivide_s32_do_vector(numers, &denom));
    }
    const int32_t *comps = (const int32_t *)&sumX4;
    int32_t sum = comps[0] + comps[1] + comps[2] + comps[3];
    return sum;
}

NOINLINE
static uint64_t mine_s32_vector_unswitched(struct FunctionParams_t *params) {
    unsigned iter;
    __m256i sumX4 = _mm256_setzero_si256();
    const struct libdivide_s32_t denom = *(struct libdivide_s32_t *)params->denomPtr;
    const int32_t *data = (const int32_t *)params->data;
    int algo = libdivide_s32_get_algorithm(&denom);
    if (algo == 0) {
        for (iter = 0; iter < ITERATIONS; iter+=8) {
            __m256i numers = _mm256_load_si256((const __m256i*)(data + iter));
            sumX4 = _mm256_add_epi32(sumX4, libdivide_s32_do_vector_alg0(numers, &denom));
        }        
    }
    else if (algo == 1) {
        for (iter = 0; iter < ITERATIONS; iter+=8) {
            __m256i numers = _mm256_load_si256((const __m256i*)(data + iter));
            sumX4 = _mm256_add_epi32(sumX4, libdivide_s32_do_vector_alg1(numers, &denom));
        }                
    }
    else if (algo == 2) {
        for (iter = 0; iter < ITERATIONS; iter+=8) {
            __m256i numers = _mm256_load_si256((const __m256i*)(data + iter));
            sumX4 = _mm256_add_epi32(sumX4, libdivide_s32_do_vector_alg2(numers, &denom));
        }                
    }
    else if (algo == 3) {
        for (iter = 0; iter < ITERATIONS; iter+=8) {
            __m256i numers = _mm256_load_si256((const __m256i*)(data + iter));
            sumX4 = _mm256_add_epi32(sumX4, libdivide_s32_do_vector_alg3(numers, &denom));
        }                
    }
    else if (algo == 4) {
        for (iter = 0; iter < ITERATIONS; iter+=8) {
            __m256i numers = _mm256_load_si256((const __m256i*)(data + iter));
            sumX4 = _mm256_add_epi32(sumX4, libdivide_s32_do_vector_alg4(numers, &denom));
        }                
    }
    const int32_t *comps = (const int32_t *)&sumX4;
    int32_t sum = comps[0] + comps[1] + comps[2] + comps[3];
    return sum;
}

NOINLINE
static uint64_t mine_s32_vector_branchfree(struct FunctionParams_t *params) {
    unsigned iter;
    __m256i sumX4 = _mm256_setzero_si256();
    const struct libdivide_s32_branchfree_t denom = *(struct libdivide_s32_branchfree_t *)params->denomBranchfreePtr;
    const int32_t *data = (const int32_t *)params->data;
    for (iter = 0; iter < ITERATIONS; iter+=8) {
        __m256i numers = _mm256_load_si256((const __m256i*)(data + iter));
        sumX4 = _mm256_add_epi32(sumX4, libdivide_s32_branchfree_do_vector(numers, &denom));
    }
    const int32_t *comps = (const int32_t *)&sumX4;
    int32_t sum = comps[0] + comps[1] + comps[2] + comps[3];
    return sum;
}

#endif

NOINLINE
static uint64_t mine_s32_unswitched(struct FunctionParams_t *params) {
    unsigned iter;
    int32_t sum = 0;
    const struct libdivide_s32_t denom = *(struct libdivide_s32_t *)params->denomPtr;
    const int32_t *data = (const int32_t *)params->data;
    int algo = libdivide_s32_get_algorithm(&denom);
    if (algo == 0) {
        for (iter = 0; iter < ITERATIONS; iter++) {
            int32_t numer = data[iter];
            sum += libdivide_s32_do_alg0(numer, &denom);
        }
    }
    else if (algo == 1) {
        for (iter = 0; iter < ITERATIONS; iter++) {
            int32_t numer = data[iter];
            sum += libdivide_s32_do_alg1(numer, &denom);
        }        
    }
    else if (algo == 2) {
        for (iter = 0; iter < ITERATIONS; iter++) {
            int32_t numer = data[iter];
            sum += libdivide_s32_do_alg2(numer, &denom);
        }        
    }
    else if (algo == 3) {
        for (iter = 0; iter < ITERATIONS; iter++) {
            int32_t numer = data[iter];
            sum += libdivide_s32_do_alg3(numer, &denom);
        }        
    }
    else if (algo == 4) {
        for (iter = 0; iter < ITERATIONS; iter++) {
            int32_t numer = data[iter];
            sum += libdivide_s32_do_alg4(numer, &denom);
        }        
    }
    
    return (uint64_t)sum;
}

NOINLINE
static uint64_t his_s32(struct FunctionParams_t *params) {
    unsigned iter;
    int32_t sum = 0;
    const int32_t d = *(int32_t *)params->d;
    const int32_t *data = (const int32_t *)params->data;
    for (iter = 0; iter < ITERATIONS; iter++) {
        int32_t numer = data[iter];
        sum += numer / d;
    }
    return sum;
}
 
NOINLINE
static uint64_t mine_s32_generate(struct FunctionParams_t *params) {
    unsigned iter;
    int32_t *dPtr = (int32_t *)params->d;
    struct libdivide_s32_t *denomPtr = (struct libdivide_s32_t *)params->denomPtr;
    for (iter = 0; iter < GEN_ITERATIONS; iter++) {
        *denomPtr = libdivide_s32_gen(*dPtr);
    }
    return *dPtr;
}
 
 
//U64
 
NOINLINE
static uint64_t mine_u64(struct FunctionParams_t *params) {
    unsigned iter;
    const struct libdivide_u64_t denom = *(struct libdivide_u64_t *)params->denomPtr;
    const uint64_t *data = (const uint64_t *)params->data;
    uint64_t sum = 0;
    for (iter = 0; iter < ITERATIONS; iter++) {
        uint64_t numer = data[iter];
        sum += libdivide_u64_do(numer, &denom);
    }
    return sum;
}

NOINLINE
static uint64_t mine_u64_branchfree(struct FunctionParams_t *params) {
    unsigned iter;
    const struct libdivide_u64_branchfree_t denom = *(struct libdivide_u64_branchfree_t *)params->denomBranchfreePtr;
    const uint64_t *data = (const uint64_t *)params->data;
    uint64_t sum = 0;
    for (iter = 0; iter < ITERATIONS; iter++) {
        uint64_t numer = data[iter];
        sum += libdivide_u64_branchfree_do(numer, &denom);
    }
    return sum;
}

NOINLINE
static uint64_t mine_u64_unswitched(struct FunctionParams_t *params) {
    unsigned iter;
    uint64_t sum = 0;
    const struct libdivide_u64_t denom = *(struct libdivide_u64_t *)params->denomPtr;
    const uint64_t *data = (const uint64_t *)params->data;
    int algo = libdivide_u64_get_algorithm(&denom);
    if (algo == 0) {
        for (iter = 0; iter < ITERATIONS; iter++) {
            uint64_t numer = data[iter];
            sum += libdivide_u64_do_alg0(numer, &denom);
        }
    }
    else if (algo == 1) {
        for (iter = 0; iter < ITERATIONS; iter++) {
            uint64_t numer = data[iter];
            sum += libdivide_u64_do_alg1(numer, &denom);
        }        
    }
    else if (algo == 2) {
        for (iter = 0; iter < ITERATIONS; iter++) {
            uint64_t numer = data[iter];
            sum += libdivide_u64_do_alg2(numer, &denom);
        }        
    }
    
    return sum;
}

#if defined(LIBDIVIDE_USE_SSE2)
NOINLINE static uint64_t mine_u64_vector_unswitched(struct FunctionParams_t *params) {
    unsigned iter;
    __m128i sumX2 = _mm_setzero_si128();
    const struct libdivide_u64_t denom = *(struct libdivide_u64_t *)params->denomPtr;
    const uint64_t *data = (const uint64_t *)params->data;
    int algo = libdivide_u64_get_algorithm(&denom);
    if (algo == 0) {
        for (iter = 0; iter < ITERATIONS; iter+=2) {
            __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
            sumX2 = _mm_add_epi64(sumX2, libdivide_u64_do_vector_alg0(numers, &denom));
        }   
    }
    else if (algo == 1) {
        for (iter = 0; iter < ITERATIONS; iter+=2) {
            __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
            sumX2 = _mm_add_epi64(sumX2, libdivide_u64_do_vector_alg1(numers, &denom));
        }   
    }
    else if (algo == 2) {
        for (iter = 0; iter < ITERATIONS; iter+=2) {
            __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
            sumX2 = _mm_add_epi64(sumX2, libdivide_u64_do_vector_alg2(numers, &denom));
        }        
    }
    const uint64_t *comps = (const uint64_t *)&sumX2;
    return comps[0] + comps[1];
}

NOINLINE static uint64_t mine_u64_vector(struct FunctionParams_t *params) {
    unsigned iter;
    __m128i sumX2 = _mm_setzero_si128();
    const struct libdivide_u64_t denom = *(struct libdivide_u64_t *)params->denomPtr;
    const uint64_t *data = (const uint64_t *)params->data;
    for (iter = 0; iter < ITERATIONS; iter+=2) {
        __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
        sumX2 = _mm_add_epi64(sumX2, libdivide_u64_do_vector(numers, &denom));
    }
    const uint64_t *comps = (const uint64_t *)&sumX2;
    return comps[0] + comps[1];
}

NOINLINE static uint64_t mine_u64_vector_branchfree(struct FunctionParams_t *params) {
    unsigned iter;
    __m128i sumX2 = _mm_setzero_si128();
    const struct libdivide_u64_branchfree_t denom = *(struct libdivide_u64_branchfree_t *)params->denomBranchfreePtr;
    const uint64_t *data = (const uint64_t *)params->data;
    for (iter = 0; iter < ITERATIONS; iter+=2) {
        __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
        sumX2 = _mm_add_epi64(sumX2, libdivide_u64_branchfree_do_vector(numers, &denom));
    }
    const uint64_t *comps = (const uint64_t *)&sumX2;
    return comps[0] + comps[1];
}
#endif

#if defined(LIBDIVIDE_USE_AVX2)
NOINLINE static uint64_t mine_u64_vector_unswitched(struct FunctionParams_t *params) {
    unsigned iter;
    __m256i sumX2 = _mm256_setzero_si256();
    const struct libdivide_u64_t denom = *(struct libdivide_u64_t *)params->denomPtr;
    const uint64_t *data = (const uint64_t *)params->data;
    int algo = libdivide_u64_get_algorithm(&denom);
    if (algo == 0) {
        for (iter = 0; iter < ITERATIONS; iter+=4) {
            __m256i numers = _mm256_load_si256((const __m256i*)(data + iter));
            sumX2 = _mm256_add_epi64(sumX2, libdivide_u64_do_vector_alg0(numers, &denom));
        }   
    }
    else if (algo == 1) {
        for (iter = 0; iter < ITERATIONS; iter+=4) {
            __m256i numers = _mm256_load_si256((const __m256i*)(data + iter));
            sumX2 = _mm256_add_epi64(sumX2, libdivide_u64_do_vector_alg1(numers, &denom));
        }   
    }
    else if (algo == 2) {
        for (iter = 0; iter < ITERATIONS; iter+=4) {
            __m256i numers = _mm256_load_si256((const __m256i*)(data + iter));
            sumX2 = _mm256_add_epi64(sumX2, libdivide_u64_do_vector_alg2(numers, &denom));
        }        
    }
    const uint64_t *comps = (const uint64_t *)&sumX2;
    return comps[0] + comps[1];
}

NOINLINE static uint64_t mine_u64_vector(struct FunctionParams_t *params) {
    unsigned iter;
    __m256i sumX2 = _mm256_setzero_si256();
    const struct libdivide_u64_t denom = *(struct libdivide_u64_t *)params->denomPtr;
    const uint64_t *data = (const uint64_t *)params->data;
    for (iter = 0; iter < ITERATIONS; iter+=4) {
        __m256i numers = _mm256_load_si256((const __m256i*)(data + iter));
        sumX2 = _mm256_add_epi64(sumX2, libdivide_u64_do_vector(numers, &denom));
    }
    const uint64_t *comps = (const uint64_t *)&sumX2;
    return comps[0] + comps[1];
}

NOINLINE static uint64_t mine_u64_vector_branchfree(struct FunctionParams_t *params) {
    unsigned iter;
    __m256i sumX2 = _mm256_setzero_si256();
    const struct libdivide_u64_branchfree_t denom = *(struct libdivide_u64_branchfree_t *)params->denomBranchfreePtr;
    const uint64_t *data = (const uint64_t *)params->data;
    for (iter = 0; iter < ITERATIONS; iter+=4) {
        __m256i numers = _mm256_load_si256((const __m256i*)(data + iter));
        sumX2 = _mm256_add_epi64(sumX2, libdivide_u64_branchfree_do_vector(numers, &denom));
    }
    const uint64_t *comps = (const uint64_t *)&sumX2;
    return comps[0] + comps[1];
}
#endif

NOINLINE
static uint64_t his_u64(struct FunctionParams_t *params) {
    unsigned iter;
    uint64_t sum = 0;
    const uint64_t d = *(uint64_t *)params->d;
    const uint64_t *data = (const uint64_t *)params->data;
    for (iter = 0; iter < ITERATIONS; iter++) {
        uint64_t numer = data[iter];
        sum += numer / d;
    }
    return sum;
}
 
NOINLINE
static uint64_t mine_u64_generate(struct FunctionParams_t *params) {
    unsigned iter;
    uint64_t *dPtr = (uint64_t *)params->d;
    struct libdivide_u64_t *denomPtr = (struct libdivide_u64_t *)params->denomPtr;
    for (iter = 0; iter < GEN_ITERATIONS; iter++) {
        *denomPtr = libdivide_u64_gen(*dPtr);
    }
    return *dPtr;
}

NOINLINE
static uint64_t mine_s64(struct FunctionParams_t *params) {
    unsigned iter;
    const struct libdivide_s64_t denom = *(struct libdivide_s64_t *)params->denomPtr;
    const int64_t *data = (const int64_t *)params->data;
    int64_t sum = 0;
    for (iter = 0; iter < ITERATIONS; iter++) {
        int64_t numer = data[iter];
        sum += libdivide_s64_do(numer, &denom);
    }
    return sum;
}

NOINLINE
static uint64_t mine_s64_branchfree(struct FunctionParams_t *params) {
    unsigned iter;
    const struct libdivide_s64_branchfree_t denom = *(struct libdivide_s64_branchfree_t *)params->denomBranchfreePtr;
    const int64_t *data = (const int64_t *)params->data;
    int64_t sum = 0;
    for (iter = 0; iter < ITERATIONS; iter++) {
        int64_t numer = data[iter];
        sum += libdivide_s64_branchfree_do(numer, &denom);
    }
    return sum;
}

#if defined(LIBDIVIDE_USE_SSE2)
NOINLINE
static uint64_t mine_s64_vector(struct FunctionParams_t *params) {
    const struct libdivide_s64_t denom = *(struct libdivide_s64_t *)params->denomPtr;
    const int64_t *data = (const int64_t *)params->data;
    
    unsigned iter;
    __m128i sumX2 = _mm_setzero_si128();
    for (iter = 0; iter < ITERATIONS; iter+=2) {
        __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
        sumX2 = _mm_add_epi64(sumX2, libdivide_s64_do_vector(numers, &denom));
    }
    const int64_t *comps = (const int64_t *)&sumX2;
    int64_t sum = comps[0] + comps[1];
    return sum;
}

NOINLINE
static uint64_t mine_s64_vector_branchfree(struct FunctionParams_t *params) {
    const struct libdivide_s64_branchfree_t denom = *(struct libdivide_s64_branchfree_t *)params->denomBranchfreePtr;
    const int64_t *data = (const int64_t *)params->data;
    
    unsigned iter;
    __m128i sumX2 = _mm_setzero_si128();
    for (iter = 0; iter < ITERATIONS; iter+=2) {
        __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
        sumX2 = _mm_add_epi64(sumX2, libdivide_s64_branchfree_do_vector(numers, &denom));
    }
    const int64_t *comps = (const int64_t *)&sumX2;
    int64_t sum = comps[0] + comps[1];
    return sum;
}

NOINLINE
static uint64_t mine_s64_vector_unswitched(struct FunctionParams_t *params) {
    const struct libdivide_s64_t denom = *(struct libdivide_s64_t *)params->denomPtr;
    const int64_t *data = (const int64_t *)params->data;
    
    unsigned iter;
    __m128i sumX2 = _mm_setzero_si128();
    int algo = libdivide_s64_get_algorithm(&denom);
    if (algo == 0) {
        for (iter = 0; iter < ITERATIONS; iter+=2) {
            __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
            sumX2 = _mm_add_epi64(sumX2, libdivide_s64_do_vector_alg0(numers, &denom));
        }        
    }
    else if (algo == 1) {
        for (iter = 0; iter < ITERATIONS; iter+=2) {
            __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
            sumX2 = _mm_add_epi64(sumX2, libdivide_s64_do_vector_alg1(numers, &denom));
        }        
    }
    else if (algo == 2) {
        for (iter = 0; iter < ITERATIONS; iter+=2) {
            __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
            sumX2 = _mm_add_epi64(sumX2, libdivide_s64_do_vector_alg2(numers, &denom));
        }        
    }
    else if (algo == 3) {
        for (iter = 0; iter < ITERATIONS; iter+=2) {
            __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
            sumX2 = _mm_add_epi64(sumX2, libdivide_s64_do_vector_alg3(numers, &denom));
        }        
    }
    else if (algo == 4) {
        for (iter = 0; iter < ITERATIONS; iter+=2) {
            __m128i numers = _mm_load_si128((const __m128i*)(data + iter));
            sumX2 = _mm_add_epi64(sumX2, libdivide_s64_do_vector_alg4(numers, &denom));
        }        
    }
    const int64_t *comps = (const int64_t *)&sumX2;
    int64_t sum = comps[0] + comps[1];
    return sum;
}
#endif

#if defined(LIBDIVIDE_USE_AVX2)
NOINLINE
static uint64_t mine_s64_vector(struct FunctionParams_t *params) {
    const struct libdivide_s64_t denom = *(struct libdivide_s64_t *)params->denomPtr;
    const int64_t *data = (const int64_t *)params->data;
    
    unsigned iter;
    __m256i sumX2 = _mm256_setzero_si256();
    for (iter = 0; iter < ITERATIONS; iter+=4) {
        __m256i numers = _mm256_load_si256((const __m256i*)(data + iter));
        sumX2 = _mm256_add_epi64(sumX2, libdivide_s64_do_vector(numers, &denom));
    }
    const int64_t *comps = (const int64_t *)&sumX2;
    int64_t sum = comps[0] + comps[1];
    return sum;
}

NOINLINE
static uint64_t mine_s64_vector_branchfree(struct FunctionParams_t *params) {
    const struct libdivide_s64_branchfree_t denom = *(struct libdivide_s64_branchfree_t *)params->denomBranchfreePtr;
    const int64_t *data = (const int64_t *)params->data;
    
    unsigned iter;
    __m256i sumX2 = _mm256_setzero_si256();
    for (iter = 0; iter < ITERATIONS; iter+=4) {
        __m256i numers = _mm256_load_si256((const __m256i*)(data + iter));
        sumX2 = _mm256_add_epi64(sumX2, libdivide_s64_branchfree_do_vector(numers, &denom));
    }
    const int64_t *comps = (const int64_t *)&sumX2;
    int64_t sum = comps[0] + comps[1];
    return sum;
}

NOINLINE
static uint64_t mine_s64_vector_unswitched(struct FunctionParams_t *params) {
    const struct libdivide_s64_t denom = *(struct libdivide_s64_t *)params->denomPtr;
    const int64_t *data = (const int64_t *)params->data;
    
    unsigned iter;
    __m256i sumX2 = _mm256_setzero_si256();
    int algo = libdivide_s64_get_algorithm(&denom);
    if (algo == 0) {
        for (iter = 0; iter < ITERATIONS; iter+=4) {
            __m256i numers = _mm256_load_si256((const __m256i*)(data + iter));
            sumX2 = _mm256_add_epi64(sumX2, libdivide_s64_do_vector_alg0(numers, &denom));
        }        
    }
    else if (algo == 1) {
        for (iter = 0; iter < ITERATIONS; iter+=4) {
            __m256i numers = _mm256_load_si256((const __m256i*)(data + iter));
            sumX2 = _mm256_add_epi64(sumX2, libdivide_s64_do_vector_alg1(numers, &denom));
        }        
    }
    else if (algo == 2) {
        for (iter = 0; iter < ITERATIONS; iter+=4) {
            __m256i numers = _mm256_load_si256((const __m256i*)(data + iter));
            sumX2 = _mm256_add_epi64(sumX2, libdivide_s64_do_vector_alg2(numers, &denom));
        }        
    }
    else if (algo == 3) {
        for (iter = 0; iter < ITERATIONS; iter+=4) {
            __m256i numers = _mm256_load_si256((const __m256i*)(data + iter));
            sumX2 = _mm256_add_epi64(sumX2, libdivide_s64_do_vector_alg3(numers, &denom));
        }        
    }
    else if (algo == 4) {
        for (iter = 0; iter < ITERATIONS; iter+=4) {
            __m256i numers = _mm256_load_si256((const __m256i*)(data + iter));
            sumX2 = _mm256_add_epi64(sumX2, libdivide_s64_do_vector_alg4(numers, &denom));
        }        
    }
    const int64_t *comps = (const int64_t *)&sumX2;
    int64_t sum = comps[0] + comps[1];
    return sum;
}
#endif

NOINLINE
static uint64_t mine_s64_unswitched(struct FunctionParams_t *params) {
    const struct libdivide_s64_t denom = *(struct libdivide_s64_t *)params->denomPtr;
    const int64_t *data = (const int64_t *)params->data;

    unsigned iter;
    int64_t sum = 0;
    int algo = libdivide_s64_get_algorithm(&denom);
    if (algo == 0) {
        for (iter = 0; iter < ITERATIONS; iter++) {
            int64_t numer = data[iter];
            sum += libdivide_s64_do_alg0(numer, &denom);
        }
    }
    else if (algo == 1) {
        for (iter = 0; iter < ITERATIONS; iter++) {
            int64_t numer = data[iter];
            sum += libdivide_s64_do_alg1(numer, &denom);
        }        
    }
    else if (algo == 2) {
        for (iter = 0; iter < ITERATIONS; iter++) {
            int64_t numer = data[iter];
            sum += libdivide_s64_do_alg2(numer, &denom);
        }        
    }
    else if (algo == 3) {
        for (iter = 0; iter < ITERATIONS; iter++) {
            int64_t numer = data[iter];
            sum += libdivide_s64_do_alg3(numer, &denom);
        }        
    }
    else if (algo == 4) {
        for (iter = 0; iter < ITERATIONS; iter++) {
            int64_t numer = data[iter];
            sum += libdivide_s64_do_alg4(numer, &denom);
        }        
    }
    
    return sum;
}
 
NOINLINE
static uint64_t his_s64(struct FunctionParams_t *params) {
    const int64_t *data = (const int64_t *)params->data;
    const int64_t d = *(int64_t *)params->d;

    unsigned iter;
    int64_t sum = 0;
    for (iter = 0; iter < ITERATIONS; iter++) {
        int64_t numer = data[iter];
        sum += numer / d;
    }
    return sum;
}
 
NOINLINE
static uint64_t mine_s64_generate(struct FunctionParams_t *params) {
    int64_t *dPtr = (int64_t *)params->d;
    struct libdivide_s64_t *denomPtr = (struct libdivide_s64_t *)params->denomPtr;
    unsigned iter;
    for (iter = 0; iter < GEN_ITERATIONS; iter++) {
        *denomPtr = libdivide_s64_gen(*dPtr);
    }
    return *dPtr;
}

/* Stub functions for when we have no SSE2 */
#if !defined(LIBDIVIDE_USE_SSE2) && \
    !defined(LIBDIVIDE_USE_AVX2)
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
 
NOINLINE
struct TestResult test_one(TestFunc_t mine, TestFunc_t mine_branchfree, TestFunc_t mine_vector, TestFunc_t mine_unswitched, TestFunc_t mine_vector_unswitched, TestFunc_t mine_vector_branchfree, TestFunc_t his, TestFunc_t generate, struct FunctionParams_t *params) {
#define TEST_COUNT 30
    struct TestResult result;
    memset(&result, 0, sizeof result);
    
#define CHECK(actual, expected) do { if (1 && actual != expected) printf("Failure on line %lu\n", (unsigned long)__LINE__); } while (0)
    
    uint64_t my_times[TEST_COUNT], my_times_branchfree[TEST_COUNT], my_times_unswitched[TEST_COUNT], my_times_vector[TEST_COUNT], my_times_vector_unswitched[TEST_COUNT], my_times_vector_branchfree[TEST_COUNT], his_times[TEST_COUNT], gen_times[TEST_COUNT];
    unsigned iter;
    struct time_result tresult;
    for (iter = 0; iter < TEST_COUNT; iter++) {
        tresult = time_function(his, params); his_times[iter] = tresult.time; const uint64_t expected = tresult.result;
        tresult = time_function(mine, params); my_times[iter] = tresult.time; CHECK(tresult.result, expected);
        tresult = time_function(mine_branchfree, params); my_times_branchfree[iter] = tresult.time; CHECK(tresult.result, expected);
        tresult = time_function(mine_unswitched, params); my_times_unswitched[iter] = tresult.time; CHECK(tresult.result, expected);
#if defined(LIBDIVIDE_USE_SSE2) || \
    defined(LIBDIVIDE_USE_AVX2)
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
        
    result.gen_time = find_min(gen_times, TEST_COUNT) / (double)GEN_ITERATIONS;
    result.my_base_time = find_min(my_times, TEST_COUNT) / (double)ITERATIONS;
    result.my_branchfree_time = find_min(my_times_branchfree, TEST_COUNT) / (double)ITERATIONS;
    result.my_vector_time = find_min(my_times_vector, TEST_COUNT) / (double)ITERATIONS;
	//printf("%f - %f\n", find_min(my_times_vector, TEST_COUNT) / (double)ITERATIONS, result.my_vector_time);
    result.my_unswitched_time = find_min(my_times_unswitched, TEST_COUNT) / (double)ITERATIONS;
    result.my_vector_branchfree_time = find_min(my_times_vector_branchfree, TEST_COUNT) / (double)ITERATIONS;
    result.my_vector_unswitched_time = find_min(my_times_vector_unswitched, TEST_COUNT) / (double)ITERATIONS;
    result.his_time = find_min(his_times, TEST_COUNT) / (double)ITERATIONS;
    return result;
#undef TEST_COUNT
}
 
NOINLINE
struct TestResult test_one_u32(uint32_t d, const uint32_t *data) {
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
 
NOINLINE
struct TestResult test_one_s32(int32_t d, const int32_t *data) {
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
 
NOINLINE
struct TestResult test_one_u64(uint64_t d, const uint64_t *data) {
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
 
NOINLINE
struct TestResult test_one_s64(int64_t d, const int64_t *data) {
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
 
static const uint32_t *random_data(unsigned multiple) {
#if defined(LIBDIVIDE_WINDOWS)
    uint32_t *data = (uint32_t *)malloc(multiple * ITERATIONS * sizeof *data);
#else
    /* Linux doesn't always give us data sufficiently aligned for SSE, so we can't use malloc(). */
    void *ptr = NULL;
    int failed = posix_memalign(&ptr, 16, multiple * ITERATIONS * sizeof(uint32_t));
    if (failed) {
        printf("Failed to align memory!\n");
        exit(1);
    }
    uint32_t *data = ptr;
#endif
    uint32_t i;
    struct random_state state = SEED;
    for (i=0; i < ITERATIONS * multiple; i++) {
        data[i] = my_random(&state);
    }
    return data;
}

int main(int argc, char* argv[]) {
#if defined(LIBDIVIDE_WINDOWS)
	QueryPerformanceFrequency(&gPerfCounterFreq);
#endif
    int i, u32 = 0, u64 = 0, s32 = 0, s64 = 0;
    if (argc == 1) {
        /* Test all */
        u32 = u64 = s32 = s64 = 1;
    }
    else {
        for (i=1; i < argc; i++) {
            if (! strcmp(argv[i], "u32")) u32 = 1;
            else if (! strcmp(argv[i], "u64")) u64 = 1;
            else if (! strcmp(argv[i], "s32")) s32 = 1;
            else if (! strcmp(argv[i], "s64")) s64 = 1;
            else printf("Unknown test '%s'\n", argv[i]), exit(0);
        }
    }
    const uint32_t *data = NULL;
    data = random_data(1);
    if (u32) test_many_u32(data);
    if (s32) test_many_s32((const int32_t *)data);
    free((void *)data);
    
    data = random_data(2);
    if (u64) test_many_u64((const uint64_t *)data);
    if (s64) test_many_s64((const int64_t *)data);
    free((void *)data);
    return 0;
}
