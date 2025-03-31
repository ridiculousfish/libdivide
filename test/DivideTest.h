#pragma once

#include <string.h>  // memcpy

#include "outputs.h"

#if defined(__AVR__)
#include <Arduino.h>

#include "avr_type_helpers.h"
typedef String string_class;
// AVR targets do not have enough memory to track which denominatores
// have been tested. So this is a dummy placeholder, just to allow
// common function signatures.
template <typename IntT>
using set_t = std::numeric_limits<IntT>;
#else
#include <limits>
#include <random>
#include <string>
typedef std::string string_class;
#include <set>
template <typename IntT>
using set_t = std::set<IntT>;
#endif

#if defined(_MSC_VER)
#pragma warning(disable : 4324)
#pragma warning(disable : 4310)
#pragma warning(disable : 4146)
#endif

#include "libdivide.h"
#include "type_mappings.h"

using namespace libdivide;

#undef UNUSED
#define UNUSED(x) (void)(x)

#if defined(LIBDIVIDE_SSE2) || defined(LIBDIVIDE_AVX2) || defined(LIBDIVIDE_AVX512) || \
    defined(LIBDIVIDE_NEON)
#define VECTOR_TESTS
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4127)  // disable "conditional expression is constant""
#endif

template <typename T>
class DivideTest {
   private:
    using UT = typename std::make_unsigned<T>::type;
    using limits = std::numeric_limits<T>;
    uint32_t seed = 0;
    UT rand_n = 0;

    // This random function slowly increases the random number
    // until there is an integer overflow, if this happens
    // the random number is reset to 0 and we restart at the
    // beginning. We do this to ensure that we get many test
    // cases (random integers) of varying bit length.
    T get_random() {
        // https://en.wikipedia.org/wiki/Linear_congruential_generator
        seed = seed * 1664525 + 1013904223;

        UT old = rand_n;
        rand_n = rand_n * (seed % 2 + 1) + rand_n % 30000001 + 3;

        // Reset upon integer overflow
        if (rand_n < old) {
            rand_n = seed % 19;
        }

        // The algorithm above generates mostly positive numbers.
        // Hence convert 50% of all values to negative.
        if (limits::is_signed) {
            if (seed % 2) return -(T)rand_n;
        }

        return (T)rand_n;
    }

    T random_denominator() {
        T denom = get_random();
        while (denom == 0) {
            denom = get_random();
        }
        return denom;
    }

    string_class testcase_name(int algo) const {
        string_class result = type_tag<T>::get_tag();
        if (algo == BRANCHFREE) {
            result += " (branchfree)";
        }
        return result;
    }

    template <Branching ALGO>
    void test_one(T numer, T denom, const divider<T, ALGO> &the_divider) {
        // Don't crash with INT_MIN / -1
        // INT_MIN / -1 is undefined behavior in C/C++
        if (limits::is_signed && numer == (limits::min)() && denom == T(-1)) {
            return;
        }

        T expect = numer / denom;
        T result = numer / the_divider;

        if (result != expect) {
            PRINT_ERROR(F("Failure for "));
            PRINT_ERROR(testcase_name(ALGO));
            PRINT_ERROR(F(": "));
            PRINT_ERROR(numer);
            PRINT_ERROR(F(" / "));
            PRINT_ERROR(denom);
            PRINT_ERROR(F(" = "));
            PRINT_ERROR(expect);
            PRINT_ERROR(F(", but got "));
            PRINT_ERROR(result);
            PRINT_ERROR(F("\n"));
            TEST_FAIL();
        }
    }

    template <typename VecType, Branching ALGO>
    void test_vec(const T *numers, size_t count, T denom, const divider<T, ALGO> &div) {
        // Number of T (E.g. in16_t) that will fit in one VecType (E.g. __m256i)
        const size_t countTinVec = sizeof(VecType) / sizeof(T);

        // Use a union to read a vector via pointer-to-integer, without violating strict
        // aliasing.
        union type_pun_vec {
            VecType vec = {};
            T arr[countTinVec];
        };

        const size_t countVec = (sizeof(T) * count) / sizeof(VecType);
        for (size_t j = 0; j < countVec; j++, numers += countTinVec) {
            type_pun_vec vec_in;
            memcpy(vec_in.arr, numers, sizeof(VecType));

            type_pun_vec vec_result;
            vec_result.vec = vec_in.vec / div;

            for (size_t i = 0; i < countTinVec; i++) {
                T numer = numers[i];
                T result = vec_result.arr[i];
                T expect = numer / denom;

                if (result != expect) {
                    PRINT_ERROR(F("Vector failure for: "));
                    PRINT_ERROR(testcase_name(ALGO));
                    PRINT_ERROR(F(": "));
                    PRINT_ERROR(numer);
                    PRINT_ERROR(F(" / "));
                    PRINT_ERROR(denom);
                    PRINT_ERROR(F(" = "));
                    PRINT_ERROR(expect);
                    PRINT_ERROR(F(", but got "));
                    PRINT_ERROR(result);
                    PRINT_ERROR(F("\n"));
                    TEST_FAIL();
                } else {
#if 0
                        std::cout << "vec" << (CHAR_BIT * sizeof(VecType)) << " success for: " << numer << " / " << denom << " = " << result << std::endl;
#endif
                }
            }
        }
    }

    // random_count * sizeof(T) must be >= size of largest
    // vector type. So figure that out at compile time.
    union vector_size_u {
        T s1;
#ifdef LIBDIVIDE_SSE2
        __m128i s2;
#endif
#ifdef LIBDIVIDE_AVX2
        __m256i s3;
#endif
#ifdef LIBDIVIDE_AVX512
        __m512i s4;
#endif
#ifdef LIBDIVIDE_NEON
        typename NeonVecFor<T>::type s5;
#endif
    };
    static const size_t min_vector_count = sizeof(union vector_size_u) / sizeof(T);

    static constexpr T min = (std::numeric_limits<T>::min)();
    static constexpr T max = (std::numeric_limits<T>::max)();
    static constexpr T edgeCases[] = {0, (T)(1), (T)(2), (T)(3), (T)(4), (T)(5), (T)(6), (T)(7),
        (T)(8), (T)(9), (T)(10), (T)(11), (T)(12), (T)(13), (T)(14), (T)(15), (T)(16), (T)(17),
        (T)(18), (T)(19), (T)(20), (T)(21), (T)(22), (T)(23), (T)(24), (T)(25), (T)(26), (T)(27),
        (T)(28), (T)(29), (T)(30), (T)(31), (T)(32), (T)(33), (T)(34), (T)(35), (T)(36), (T)(37),
        (T)(38), (T)(39), (T)(40), (T)(41), (T)(42), (T)(43), (T)(44), (T)(45), (T)(46), (T)(47),
        (T)(48), (T)(49), (T)(123), (T)(1232), (T)(36847), (T)(506838), (T)(3000003), (T)(70000007),

        (T)(max), (T)(max - 1), (T)(max - 2), (T)(max - 3), (T)(max - 4), (T)(max - 5),
        (T)(max - 3213), (T)(max - 2453242), (T)(max - 432234231),

        (T)(min), (T)(min + 1), (T)(min + 2), (T)(min + 3), (T)(min + 4), (T)(min + 5),
        (T)(min + 3213), (T)(min + 2453242), (T)(min + 432234231),

        (T)(max / 2), (T)(max / 2 + 1), (T)(max / 2 - 1), (T)(max / 3), (T)(max / 3 + 1),
        (T)(max / 3 - 1), (T)(max / 4), (T)(max / 4 + 1), (T)(max / 4 - 1),

        (T)(min / 2), (T)(min / 2 + 1), (T)(min / 2 - 1), (T)(min / 3), (T)(min / 3 + 1),
        (T)(min / 3 - 1), (T)(min / 4), (T)(min / 4 + 1), (T)(min / 4 - 1)};

    template <Branching ALGO>
    void test_edgecase_numerators(T denom, const divider<T, ALGO> &the_divider) {
        for (auto numerator : edgeCases) {
            test_one((T)numerator, denom, the_divider);
        }
    }

    template <Branching ALGO>
    void test_small_numerators(T denom, const divider<T, ALGO> &the_divider) {
        // test small numerators < 2^16
        // balance signed & unsigned testing
#if defined(__AVR__)
        int32_t small_stop = (limits::is_signed) ? (int32_t)1 << 7 : (uint32_t)1 << 8;
#else
        int32_t small_stop = (limits::is_signed) ? (int32_t)1 << 14 : (uint32_t)1 << 16;
#endif
        for (int32_t i = 0; i < small_stop; ++i) {
            test_one((T)i, denom, the_divider);

            if (limits::is_signed) {
                test_one((T)-i, denom, the_divider);
            }
        }
    }

    template <Branching ALGO>
    void test_pow2_numerators(T denom, const divider<T, ALGO> &the_divider) {
        // test power of 2 numerators: 2^i-1, 2^i, 2^i+1
        for (int i = 1; i < limits::digits; i++) {
            for (int j = -1; j <= 1; j++) {
                T numerator = static_cast<T>((static_cast<T>(1) << i) + j);
                test_one(numerator, denom, the_divider);

                if (limits::is_signed) {
                    test_one(-numerator, denom, the_divider);
                }
            }
        }
    }

    template <Branching ALGO>
    void test_allbits_numerators(T denom, const divider<T, ALGO> &the_divider) {
        // test all bits set:
        // 11111111, 11111110, 11111100, ...
        for (UT bits = (std::numeric_limits<UT>::max)(); bits != 0; bits <<= 1) {
            test_one((T)bits, denom, the_divider);
        }
    }

    template <Branching ALGO>
    void test_random_numerators(T denom, const divider<T, ALGO> &the_divider) {
        for (size_t i = 0; i < 10000; ++i) {
            for (size_t j = 0; j < min_vector_count; j++) {
                test_one(get_random(), denom, the_divider);
            }
        }
    }

    template <Branching ALGO>
    void test_vectordivide_numerators(T denom, const divider<T, ALGO> &the_divider) {
#if defined(VECTOR_TESTS)
        // Align memory to 64 byte boundary for AVX512
        char mem[min_vector_count * sizeof(T) + 64];
        size_t offset = 64 - (size_t)&mem % 64;
        T *numers = (T *)&mem[offset];

        for (size_t i = 0; i < 10000; ++i) {
            for (size_t j = 0; j < min_vector_count; j++) {
                numers[j] = get_random();
            }
#ifdef LIBDIVIDE_SSE2
            test_vec<__m128i>(numers, min_vector_count, denom, the_divider);
#endif
#ifdef LIBDIVIDE_AVX2
            test_vec<__m256i>(numers, min_vector_count, denom, the_divider);
#endif
#ifdef LIBDIVIDE_AVX512
            test_vec<__m512i>(numers, min_vector_count, denom, the_divider);
#endif
#ifdef LIBDIVIDE_NEON
            test_vec<typename NeonVecFor<T>::type>(numers, min_vector_count, denom, the_divider);
#endif
        }
#else
        UNUSED(denom);
        UNUSED(the_divider);
#endif
    }

    template <Branching ALGO>
    void test_all_numerators(T denom, const divider<T, ALGO> &the_divider) {
        for (T numerator = (min); numerator != (max); ++numerator) {
            test_one((T)numerator, denom, the_divider);
        }
    }

    template <Branching ALGO>
    void test_many(T denom) {
        // Don't try dividing by 1 with unsigned branchfree
        if (ALGO == BRANCHFREE && !std::numeric_limits<T>::is_signed && denom == 1) {
            return;
        }

        const divider<T, ALGO> the_divider = divider<T, ALGO>(denom);
        T recovered = the_divider.recover();
        if (recovered != denom) {
            PRINT_ERROR(F("Failed to recover divisor for "));
            PRINT_ERROR(testcase_name(ALGO));
            PRINT_ERROR(F(": "));
            PRINT_ERROR(denom);
            PRINT_ERROR(F(", but got "));
            PRINT_ERROR(recovered);
            PRINT_ERROR(F("\n"));
            TEST_FAIL();
        }

        test_edgecase_numerators(denom, the_divider);
        test_small_numerators(denom, the_divider);
        test_pow2_numerators(denom, the_divider);
        test_allbits_numerators(denom, the_divider);
#if !defined(__AVR__)
        test_random_numerators(denom, the_divider);
        test_vectordivide_numerators(denom, the_divider);
#endif
    }

    static uint32_t randomSeed() {
#if defined(__AVR__)
        return (uint32_t)analogRead(A0);
#else
        std::random_device randomDevice;
        std::mt19937 randGen(randomDevice());
        std::uniform_int_distribution<uint32_t> randDist(1, std::numeric_limits<uint32_t>::max());
        return randDist(randGen);
#endif
    }

    void test_all_algorithms(T denom, set_t<T> &tested_denom) {
#if !defined(__AVR__)
        if (tested_denom.end() == tested_denom.find(denom)) {
#endif
            PRINT_PROGRESS_MSG(F("Testing deom "));
            PRINT_PROGRESS_MSG(denom);
            PRINT_PROGRESS_MSG(F("\n"));
            test_many<BRANCHFULL>(denom);
            test_many<BRANCHFREE>(denom);
#if !defined(__AVR__)
            tested_denom.insert(denom);
        }
#else
        UNUSED(tested_denom);
#endif
    }

    void test_both_signs(UT denom, set_t<T> &tested_denom) {
        test_all_algorithms(denom, tested_denom);

        if (limits::is_signed) {
            test_all_algorithms(-denom, tested_denom);
        }
    }

   public:
    DivideTest() {
        seed = randomSeed();
        rand_n = (UT)randomSeed();
    }

    void run() {
        set_t<T> tested_denom;

        // Test small values
#if defined(__AVR__)
        UT primes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
            73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163,
            167, 173};
        for (size_t index = 0; index < sizeof(primes) / sizeof(primes[0]); ++index) {
            test_both_signs(primes[index], tested_denom);
        }
#else
        for (UT denom = 1; denom < 1024; ++denom) {
            test_both_signs(denom, tested_denom);
        }
#endif

        if (limits::is_signed) {
            PRINT_PROGRESS_MSG(F("Testing minimum\n"));
            test_all_algorithms((limits::min)(), tested_denom);
        }

        PRINT_PROGRESS_MSG(F("Testing maximum\n"));
        test_all_algorithms((limits::max)(), tested_denom);

        // test power of 2 denoms: 2^i-1, 2^i, 2^i+1
        PRINT_PROGRESS_MSG(F("Testing powers of 2\n"));
        for (int i = 1; i < limits::digits; i++) {
            for (int j = -1; j <= 1; j++) {
                T denom = static_cast<UT>((static_cast<T>(1) << i) + j);

                test_both_signs(denom, tested_denom);
            }
        }

        // test all bits set:
        // 11111111, 11111110, 11111100, ...
        PRINT_PROGRESS_MSG(F("Testing all bits set\n"));
        // For signed types, this degenerates to negative powers of
        // 2 (-1, -2, -4....): since we just tested those (above), skip.
        if (!limits::is_signed) {
            for (UT bits = (std::numeric_limits<UT>::max)(); bits != 0; bits <<= 1) {
                PRINT_PROGRESS_MSG((T)bits);
                PRINT_PROGRESS_MSG("\n");
                test_all_algorithms((T)bits, tested_denom);
            }
        }

        // Test random denominators
#if !defined(__AVR__)
        PRINT_PROGRESS_MSG(F("Test random denominators\n"));
        for (int i = 0; i < 10000; ++i) {
            test_all_algorithms(random_denominator(), tested_denom);
        }
#endif
    }
};

template <typename IntT>
constexpr IntT DivideTest<IntT>::edgeCases[];

template <typename T>
void run_test() {
    char msg[64];
    snprintf(msg, sizeof(msg), "Testing %s\n", type_name<T>::get_name());
    PRINT_INFO(msg);
    DivideTest<T> dt;
    dt.run();
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
