// Unit tests for divlu and divllu.
// To build standalone:
//   cc -O1 -o test_divlu test/test_divlu.c doc/divlu.c

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

uint32_t divlu(uint32_t numhi, uint32_t numlo, uint32_t den, uint32_t *r);
uint64_t divllu(uint64_t numhi, uint64_t numlo, uint64_t den, uint64_t *r);

typedef unsigned long long ullong;

static void check32(
    uint32_t numhi, uint32_t numlo, uint32_t den, uint32_t expected_quot, uint32_t expected_rem) {
    uint32_t rem;
    uint32_t quot = divlu(numhi, numlo, den, &rem);
    if (quot != expected_quot || rem != expected_rem) {
        fprintf(stderr, "divlu(%u, %u, %u): got q=%u r=%u, expected q=%u r=%u\n", numhi, numlo, den,
            quot, rem, expected_quot, expected_rem);
        abort();
    }
}

static void check64(
    uint64_t numhi, uint64_t numlo, uint64_t den, uint64_t expected_quot, uint64_t expected_rem) {
    uint64_t rem;
    uint64_t quot = divllu(numhi, numlo, den, &rem);
    if (quot != expected_quot || rem != expected_rem) {
        fprintf(stderr, "divllu(%llu, %llu, %llu): got q=%llu r=%llu, expected q=%llu r=%llu\n",
            (ullong)numhi, (ullong)numlo, (ullong)den, (ullong)quot, (ullong)rem,
            (ullong)expected_quot, (ullong)expected_rem);
        abort();
    }
}

// Compute expected result from reference arithmetic and check.
static void verify32(uint32_t numhi, uint32_t numlo, uint32_t den) {
    uint32_t expected_quot, expected_rem;
    if (numhi >= den) {
        expected_quot = UINT32_MAX;
        expected_rem = UINT32_MAX;
    } else {
        uint64_t num = ((uint64_t)numhi << 32) | numlo;
        expected_quot = (uint32_t)(num / den);
        expected_rem = (uint32_t)(num % den);
    }
    check32(numhi, numlo, den, expected_quot, expected_rem);
}

// Compute hi:lo = a * b as a 128-bit product.
static void mul64(uint64_t a, uint64_t b, uint64_t *phi, uint64_t *plo) {
    uint64_t alo = (uint32_t)a, ahi = a >> 32;
    uint64_t blo = (uint32_t)b, bhi = b >> 32;
    uint64_t ll = alo * blo;
    uint64_t lh = alo * bhi;
    uint64_t hl = ahi * blo;
    uint64_t hh = ahi * bhi;
    uint64_t mid = (ll >> 32) + (uint32_t)lh + (uint32_t)hl;
    *phi = hh + (lh >> 32) + (hl >> 32) + (mid >> 32);
    *plo = (mid << 32) | (uint32_t)ll;
}

// Rather than computing the expected value with 128-bit arithmetic, verify
// the invariant: rem < den and quot * den + rem == numhi:numlo.
static void verify64(uint64_t numhi, uint64_t numlo, uint64_t den) {
    uint64_t rem;
    uint64_t quot = divllu(numhi, numlo, den, &rem);
    if (numhi >= den) {
        if (quot != UINT64_MAX || rem != UINT64_MAX) {
            fprintf(stderr, "divllu(%llu, %llu, %llu): expected overflow, got q=%llu r=%llu\n",
                (ullong)numhi, (ullong)numlo, (ullong)den, (ullong)quot, (ullong)rem);
            abort();
        }
        return;
    }
    if (rem >= den) {
        fprintf(stderr, "divllu(%llu, %llu, %llu): rem=%llu out of range (den=%llu)\n",
            (ullong)numhi, (ullong)numlo, (ullong)den, (ullong)rem, (ullong)den);
        abort();
    }
    // Check quot * den + rem == numhi:numlo.
    uint64_t phi, plo;
    mul64(quot, den, &phi, &plo);
    plo += rem;
    if (plo < rem) phi++;
    if (phi != numhi || plo != numlo) {
        fprintf(stderr, "divllu(%llu, %llu, %llu): q=%llu r=%llu fails reconstruction\n",
            (ullong)numhi, (ullong)numlo, (ullong)den, (ullong)quot, (ullong)rem);
        abort();
    }
}

int main(void) {
    // Trivial
    check32(0, 0, 1, 0, 0);
    check32(0, 1, 1, 1, 0);
    check32(0, 7, 3, 2, 1);
    // numlo == denom
    check32(0, UINT32_MAX, UINT32_MAX, 1, 0);
    // Divide across the 32-bit boundary: (1<<32) / 2 == 1<<31
    check32(1, 0, 2, (uint32_t)1 << 31, 0);
    check32(1, 1, 2, (uint32_t)1 << 31, 1);
    // Near-maximum inputs
    check32(UINT32_MAX - 1, 0, UINT32_MAX, UINT32_MAX - 1, UINT32_MAX - 1);
    check32(UINT32_MAX - 1, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX - 1);
    // Overflow: numhi >= den
    check32(1, 0, 1, UINT32_MAX, UINT32_MAX);
    check32(UINT32_MAX, 0, UINT32_MAX, UINT32_MAX, UINT32_MAX);
    // Divide by zero: numhi (0) >= den (0)
    check32(0, 1, 0, UINT32_MAX, UINT32_MAX);

    // --- divllu hardcoded cases ---

    check64(0, 0, 1, 0, 0);
    check64(0, 1, 1, 1, 0);
    check64(0, 5, 3, 1, 2);
    check64(0, UINT64_MAX, UINT64_MAX, 1, 0);
    // Divide across the 64-bit boundary
    check64(1, 0, 2, (uint64_t)1 << 63, 0);
    check64(1, 1, 2, (uint64_t)1 << 63, 1);
    // Near-maximum inputs
    check64(UINT64_MAX - 1, 0, UINT64_MAX, UINT64_MAX - 1, UINT64_MAX - 1);
    check64(UINT64_MAX - 1, UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX - 1);
    // Overflow
    check64(1, 0, 1, UINT64_MAX, UINT64_MAX);
    check64(UINT64_MAX, 0, UINT64_MAX, UINT64_MAX, UINT64_MAX);
    // Divide by zero
    check64(0, 1, 0, UINT64_MAX, UINT64_MAX);

    // NULL remainder pointer: quotient must still be correct.
    if (divlu(0, 7, 3, NULL) != 2) abort();
    if (divllu(0, 5, 3, NULL) != 1) abort();

    // --- Cases that trigger the qhat -= 1 correction ---
    //
    // The algorithm estimates the quotient digit qhat as (numhi / den1) where
    // den1 is the high half of the denominator. When den0 (the low half) is
    // large this estimate can be off by one. The correction fires when
    // qhat*den0 > rhat*base + num1.
    //
    // divlu (base 2^16): den=0x8000FFFF gives den1=0x8000, den0=0xFFFF.
    // With numhi=0x7FFFFFFF: qhat=0xFFFF, rhat=0x7FFF,
    //   qhat*den0 = 0xFFFE0001 > rhat*base = 0x7FFF0000 → correction fires.
    verify32(0x7FFFFFFFu, 0, 0x8000FFFFu);
    verify32(0x7FFFFFFFu, 0xFFFF0000u, 0x8000FFFFu);
    //
    // divllu (base 2^32): den=0x80000000FFFFFFFF gives den1=0x80000000, den0=0xFFFFFFFF.
    // With numhi=0x7FFFFFFFFFFFFFFF: qhat=0xFFFFFFFF, rhat=0x7FFFFFFF,
    //   qhat*den0 = 0xFFFFFFFE00000001 > rhat*base = 0x7FFFFFFF00000000 → correction fires.
    verify64(UINT64_C(0x7FFFFFFFFFFFFFFF), 0, UINT64_C(0x80000000FFFFFFFF));
    verify64(
        UINT64_C(0x7FFFFFFFFFFFFFFF), UINT64_C(0xFFFFFFFF00000000), UINT64_C(0x80000000FFFFFFFF));

    // --- Systematic sweeps ---

    // divlu: all combinations with den, numhi, numlo < 256 (~8M calls).
    for (uint32_t den = 1; den < 256; den++)
        for (uint32_t numhi = 0; numhi < den; numhi++)
            for (uint32_t numlo = 0; numlo < 256; numlo++) verify32(numhi, numlo, den);

    // divlu: medium denominators, probing numhi at 0, midpoint, and max (~353M calls).
    for (uint32_t den = 256; den < 0x70000; den++)
        for (uint32_t numlo = 0; numlo < 256; numlo++) {
            verify32(0, numlo, den);
            verify32(den / 2, numlo, den);
            verify32(den - 1, numlo, den);
        }

    // divlu: near UINT32_MAX
    for (uint32_t d = 0; d < 256; d++) {
        uint32_t den = UINT32_MAX - d;
        for (uint32_t numlo = 0; numlo < 256; numlo++) {
            verify32(den - 1, numlo, den);
            verify32(den - 1, UINT32_MAX - numlo, den);
        }
    }

    // divllu: all combinations with den, numhi, numlo < 64
    for (uint64_t den = 1; den < 64; den++)
        for (uint64_t numhi = 0; numhi < den; numhi++)
            for (uint64_t numlo = 0; numlo < 64; numlo++) verify64(numhi, numlo, den);

    // divllu: medium denominators
    for (uint64_t den = 64; den < 0x70000; den++)
        for (uint64_t numlo = 0; numlo < 256; numlo++) {
            verify64(0, numlo, den);
            verify64(den / 2, numlo, den);
            verify64(den - 1, numlo, den);
        }

    return 0;
}
