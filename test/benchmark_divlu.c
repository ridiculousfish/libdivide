// Benchmark for divlu and divllu.
// Runs continuously until interrupted. All times are ns/call.
// To build standalone:
//   cc -O2 -o benchmark_divlu test/benchmark_divlu.c doc/divlu.c

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

uint32_t divlu(uint32_t numhi, uint32_t numlo, uint32_t den, uint32_t *r);
uint64_t divllu(uint64_t numhi, uint64_t numlo, uint64_t den, uint64_t *r);

#define ARRAY_LEN 4096
#define NTRIALS 5000

typedef unsigned long long ullong;

static uint64_t now_ns(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (uint64_t)ts.tv_sec * UINT64_C(1000000000) + (uint64_t)ts.tv_nsec;
}

// xorshift64 RNG.
static uint64_t rng = UINT64_C(0x1234567890ABCDEF);
static uint64_t next_rand(void) {
    rng ^= rng << 13;
    rng ^= rng >> 7;
    rng ^= rng << 17;
    return rng;
}

static uint32_t numhi32[ARRAY_LEN];
static uint32_t numlo32[ARRAY_LEN];
static uint64_t numhi64[ARRAY_LEN];
static uint64_t numlo64[ARRAY_LEN];

static void fill32(uint32_t den) {
    uint32_t i;
    for (i = 0; i < ARRAY_LEN; i++) {
        numhi32[i] = (uint32_t)(next_rand() % den);
        numlo32[i] = (uint32_t)next_rand();
    }
}

static void fill64(uint64_t den) {
    uint32_t i;
    for (i = 0; i < ARRAY_LEN; i++) {
        numhi64[i] = next_rand() % den;
        numlo64[i] = next_rand();
    }
}

// Hardware reference for divlu: a single 64-bit divide and modulo.
static uint64_t run_hw64(uint32_t den) {
    uint64_t sum = 0;
    uint32_t i;
    for (i = 0; i < ARRAY_LEN; i++) {
        uint64_t num = ((uint64_t)numhi32[i] << 32) | numlo32[i];
        sum += num / den + num % den;
    }
    return sum;
}

static uint64_t run_divlu(uint32_t den) {
    uint64_t sum = 0;
    uint32_t i;
    for (i = 0; i < ARRAY_LEN; i++) {
        uint32_t rem;
        sum += divlu(numhi32[i], numlo32[i], den, &rem);
        sum += rem;
    }
    return sum;
}

static uint64_t run_divllu(uint64_t den) {
    uint64_t sum = 0;
    uint32_t i;
    for (i = 0; i < ARRAY_LEN; i++) {
        uint64_t rem;
        sum += divllu(numhi64[i], numlo64[i], den, &rem);
        sum += rem;
    }
    return sum;
}

// Hardware 128/64->64 narrowing divide using __uint128_t.
// clang-cl on Windows does not support 128-bit division (same guard as libdivide.h).
#if defined(__SIZEOF_INT128__) && !(defined(__clang__) && defined(_MSC_VER))
static uint64_t run_hw128(uint64_t den) {
    typedef unsigned __int128 u128;
    uint64_t sum = 0;
    uint32_t i;
    for (i = 0; i < ARRAY_LEN; i++) {
        u128 num = ((u128)numhi64[i] << 64) | numlo64[i];
        sum += (uint64_t)(num / den) + (uint64_t)(num % den);
    }
    return sum;
}
#endif

// Prevent the compiler from discarding results.
static volatile uint64_t sink;

// Time FUNC NTRIALS times, store minimum ns/call in RESULT and last return value in RETVAL.
#define TIME_FUNC(result, retval, func) do {                             \
    uint64_t _min_ns = UINT64_MAX;                                       \
    int _t;                                                              \
    for (_t = 0; _t < NTRIALS; _t++) {                                  \
        uint64_t _t0 = now_ns();                                         \
        (retval) = func;                                                 \
        uint64_t _t1 = now_ns();                                         \
        uint64_t _elapsed = _t1 - _t0;                                  \
        if (_elapsed < _min_ns) _min_ns = _elapsed;                     \
    }                                                                    \
    sink = (retval);                                                     \
    (result) = _min_ns / (double)ARRAY_LEN;                             \
} while (0)

int main(void) {
    uint32_t den = 1;
    printf("%10s %10s %10s %10s %10s\n", "den", "divlu", "hw(64b)", "divllu", "hw(128b)");
    while (1) {
        double t_divlu, t_hw64, t_divllu, t_hw128;
        uint64_t r_divlu, r_hw64, r_divllu, r_hw128;

        fill32(den);
        TIME_FUNC(t_divlu, r_divlu, run_divlu(den));
        TIME_FUNC(t_hw64,  r_hw64,  run_hw64(den));
        if (r_divlu != r_hw64) {
            fprintf(stderr, "divlu mismatch for den=%u: divlu=%llu hw64=%llu\n",
                    den, (ullong)r_divlu, (ullong)r_hw64);
            abort();
        }

        fill64(den);
        TIME_FUNC(t_divllu, r_divllu, run_divllu(den));
#if defined(__SIZEOF_INT128__) && !(defined(__clang__) && defined(_MSC_VER))
        TIME_FUNC(t_hw128, r_hw128, run_hw128(den));
        if (r_divllu != r_hw128) {
            fprintf(stderr, "divllu mismatch for den=%llu: divllu=%llu hw128=%llu\n",
                    (ullong)den, (ullong)r_divllu, (ullong)r_hw128);
            abort();
        }
#else
        t_hw128 = 0;
#endif

        printf("%10u %10.3f %10.3f %10.3f %10.3f\n",
               den, t_divlu, t_hw64, t_divllu, t_hw128);
        fflush(stdout);

        den = (den == UINT32_MAX) ? 1 : den + 1;
    }
}
