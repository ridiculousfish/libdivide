// Tests divlu and divllu, intended for use with AFL.
// This aborts on error.
// To build:
//   clang divlu_fuzz_driver.cpp divlu.c

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

extern "C" {
uint32_t divlu(uint32_t numhi, uint32_t numlo, uint32_t den, uint32_t *r);
uint64_t divllu(uint64_t numhi, uint64_t numlo, uint64_t den, uint64_t *r);
}

// Read one uint64.
static uint64_t read1() {
    unsigned long long res = UINT64_MAX;
    // We may read either raw bytes (faster fuzzing)
    // or text (slower fuzzing but easier to understand test cases).
    bool raw_bytes = true;
    if (raw_bytes) {
        if (fread(&res, sizeof res, 1, stdin) != 1) {
            exit(EXIT_FAILURE);
        }
    } else {
        if (scanf(" %llu ", &res) != 1) {
            exit(EXIT_FAILURE);
        }
    }
    return res;
}

static void test64(uint64_t numhi, uint64_t numlo, uint64_t denom) {
    typedef unsigned long long ullong;

    uint64_t rem;
    uint64_t quot = divllu(numhi, numlo, denom, &rem);

    uint64_t expected_rem;
    uint64_t expected_quot;
    if (numhi >= denom) {
        expected_rem = UINT64_MAX;
        expected_quot = UINT64_MAX;
    } else {
        __uint128_t num = (__uint128_t(numhi) << 64) | numlo;
        expected_quot = num / denom;
        expected_rem = num % denom;
    }
    if (expected_quot != quot) {
        fprintf(stderr, "divllu: %llu %llu / %llu -> got %llu, expected %llu\n", ullong(numhi),
            ullong(numlo), ullong(denom), ullong(quot), ullong(expected_quot));
        abort();
    }
    if (expected_rem != rem) {
        fprintf(stderr, "divllu: %llu %llu %% %llu -> got %llu, expected %llu\n", ullong(numhi),
            ullong(numlo), ullong(denom), ullong(rem), ullong(expected_rem));
        abort();
    }
}

static void test32(uint32_t numhi, uint32_t numlo, uint32_t denom) {
    typedef unsigned long long ullong;
    uint32_t rem;
    uint32_t quot = divlu(numhi, numlo, denom, &rem);

    uint32_t expected_rem;
    uint32_t expected_quot;
    if (numhi >= denom) {
        expected_rem = UINT32_MAX;
        expected_quot = UINT32_MAX;
    } else {
        uint64_t num = (uint64_t(numhi) << 32) | numlo;
        expected_quot = num / denom;
        expected_rem = num % denom;
    }
    if (expected_quot != quot) {
        fprintf(stderr, "divlu: %llu %llu / %llu -> got %llu, expected %llu\n", ullong(numhi),
            ullong(numlo), ullong(denom), ullong(quot), ullong(expected_quot));
        abort();
    }
    if (expected_rem != rem) {
        fprintf(stderr, "divlu: %llu %llu %% %llu -> got %llu, expected %llu\n", ullong(numhi),
            ullong(numlo), ullong(denom), ullong(rem), ullong(expected_rem));
        abort();
    }
}

int main(void) {
    uint64_t denom = read1();
    for (;;) {
        uint64_t numlo = read1();
        uint64_t numhi = read1();
        test32((uint32_t)numhi, (uint32_t)numlo, (uint32_t)denom);
        test64(numhi, numlo, denom);
    }
}
