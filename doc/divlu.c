#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

/*
 * Perform a narrowing division: 128 / 64 -> 64, and 64 / 32 -> 32.
 * The dividend's low and high words are given by \p numhi and \p numlo, respectively.
 * The divisor is given by \p den.
 * \return the quotient, and the remainder by reference in \p r, if not null.
 * If the quotient would require more than 64 bits, or if denom is 0, then return the max value
 * for both quotient and remainder.
 *
 * These functions are released into the public domain, where applicable, or the CC0 license.
 */
uint64_t divllu(uint64_t numhi, uint64_t numlo, uint64_t den, uint64_t *r)
{
    // We work in base 2**32.
    // A uint32 holds a single digit. A uint64 holds two digits.
    // Our numerator is conceptually [num3, num2, num1, num0].
    // Our denominator is [den1, den0].
    const uint64_t b = (1ull << 32);

    // The high and low digits of our computed quotient.
    uint32_t q1;
    uint32_t q0;

    // The normalization shift factor.
    int shift;

    // The high and low digits of our denominator (after normalizing).
    // Also the low 2 digits of our numerator (after normalizing).
    uint32_t den1;
    uint32_t den0;
    uint32_t num1;
    uint32_t num0;

    // A partial remainder.
    uint64_t rem;

    // The estimated quotient, and its corresponding remainder (unrelated to true remainder).
    uint64_t qhat;
    uint64_t rhat;

    // Variables used to correct the estimated quotient.
    uint64_t c1;
    uint64_t c2;

    // Check for overflow and divide by 0.
    if (numhi >= den) {
        if (r != NULL)
            *r = ~0ull;
        return ~0ull;
    }

    // Determine the normalization factor. We multiply den by this, so that its leading digit is at
    // least half b. In binary this means just shifting left by the number of leading zeros, so that
    // there's a 1 in the MSB.
    // We also shift numer by the same amount. This cannot overflow because numhi < den.
    // The expression (-shift & 63) is the same as (64 - shift), except it avoids the UB of shifting
    // by 64. The funny bitwise 'and' ensures that numlo does not get shifted into numhi if shift is 0.
    // clang 11 has an x86 codegen bug here: see LLVM bug 50118. The sequence below avoids it.
    shift = __builtin_clzll(den);
    den <<= shift;
    numhi <<= shift;
    numhi |= (numlo >> (-shift & 63)) & (-(int64_t)shift >> 63);
    numlo <<= shift;

    // Extract the low digits of the numerator and both digits of the denominator.
    num1 = (uint32_t)(numlo >> 32);
    num0 = (uint32_t)(numlo & 0xFFFFFFFFu);
    den1 = (uint32_t)(den >> 32);
    den0 = (uint32_t)(den & 0xFFFFFFFFu);

    // We wish to compute q1 = [n3 n2 n1] / [d1 d0].
    // Estimate q1 as [n3 n2] / [d1], and then correct it.
    // Note while qhat may be 2 digits, q1 is always 1 digit.
    qhat = numhi / den1;
    rhat = numhi % den1;
    c1 = qhat * den0;
    c2 = rhat * b + num1;
    if (c1 > c2)
        qhat -= (c1 - c2 > den) ? 2 : 1;
    q1 = (uint32_t)qhat;

    // Compute the true (partial) remainder.
    rem = numhi * b + num1 - q1 * den;

    // We wish to compute q0 = [rem1 rem0 n0] / [d1 d0].
    // Estimate q0 as [rem1 rem0] / [d1] and correct it.
    qhat = rem / den1;
    rhat = rem % den1;
    c1 = qhat * den0;
    c2 = rhat * b + num0;
    if (c1 > c2)
        qhat -= (c1 - c2 > den) ? 2 : 1;
    q0 = (uint32_t)qhat;

    // Return remainder if requested.
    if (r != NULL)
        *r = (rem * b + num0 - q0 * den) >> shift;
    return ((uint64_t)q1 << 32) | q0;
}

uint32_t divlu(uint32_t numhi, uint32_t numlo, uint32_t den, uint32_t *r)
{
    // We work in base 2**32.
    // A uint16 holds a single digit. A uint32 holds two digits.
    // Our numerator is conceptually [num3, num2, num1, num0].
    // Our denominator is [den1, den0].
    const uint32_t b = (1ull << 16);

    // The high and low digits of our computed quotient.
    uint16_t q1;
    uint16_t q0;

    // The normalization shift factor.
    int shift;

    // The high and low digits of our denominator (after normalizing).
    // Also the low 2 digits of our numerator (after normalizing).
    uint16_t den1;
    uint16_t den0;
    uint16_t num1;
    uint16_t num0;

    // A partial remainder.
    uint32_t rem;

    // The estimated quotient, and its corresponding remainder (unrelated to true remainder).
    uint32_t qhat;
    uint32_t rhat;

    // Variables used to correct the estimated quotient.
    uint32_t c1;
    uint32_t c2;

    // Check for overflow and divide by 0.
    if (numhi >= den) {
        if (r != NULL)
            *r = ~0u;
        return ~0u;
    }

    // Determine the normalization factor. We multiply den by this, so that its leading digit is at
    // least half b. In binary this means just shifting left by the number of leading zeros, so that
    // there's a 1 in the MSB.
    // We also shift numer by the same amount. This cannot overflow because numhi < den.
    // The expression (-shift & 31) is the same as (32 - shift), except it avoids the UB of shifting
    // by 32. The funny bitwise 'and' ensures that numlo does not get shifted into numhi if shift is 0.
    // clang 11 has an x86 codegen bug here: see LLVM bug 50118. The sequence below avoids it.
    shift = __builtin_clz(den);
    den <<= shift;
    numhi <<= shift;
    numhi |= (numlo >> (-shift & 31)) & (-(int32_t)shift >> 31);
    numlo <<= shift;

    // Extract the low digits of the numerator and both digits of the denominator.
    num1 = (uint16_t)(numlo >> 16);
    num0 = (uint16_t)(numlo & 0xFFFFu);
    den1 = (uint16_t)(den >> 16);
    den0 = (uint16_t)(den & 0xFFFFu);

    // We wish to compute q1 = [n3 n2 n1] / [d1 d0].
    // Estimate q1 as [n3 n2] / [d1], and then correct it.
    // Note while qhat may be 2 digits, q1 is always 1 digit.
    qhat = numhi / den1;
    rhat = numhi % den1;
    c1 = qhat * den0;
    c2 = rhat * b + num1;
    if (c1 > c2)
        qhat -= (c1 - c2 > den) ? 2 : 1;
    q1 = (uint16_t)qhat;

    // Compute the true (partial) remainder.
    rem = numhi * b + num1 - q1 * den;

    // We wish to compute q0 = [rem1 rem0 n0] / [d1 d0].
    // Estimate q0 as [rem1 rem0] / [d1] and correct it.
    qhat = rem / den1;
    rhat = rem % den1;
    c1 = qhat * den0;
    c2 = rhat * b + num0;
    if (c1 > c2)
        qhat -= (c1 - c2 > den) ? 2 : 1;
    q0 = (uint16_t)qhat;

    // Return remainder if requested.
    if (r != NULL)
        *r = (rem * b + num0 - q0 * den) >> shift;
    return ((uint32_t)q1 << 16) | q0;
}

