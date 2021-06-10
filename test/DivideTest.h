#pragma once

#include "outputs.h"

#if defined(__AVR__)
#include <Arduino.h>
#include "avr_type_helpers.h"
typedef String string_class;
#else
#include <limits>
#include <random>
#include <string>
typedef std::string string_class;
#endif

#include "libdivide.h"
#include "type_mappings.h"

using namespace libdivide;

#define UNUSED(x) (void)(x)

template <typename T>
class DivideTest {
   private:
    using UT = typename std::make_unsigned<T>::type;
    using limits = std::numeric_limits<T>;
    uint32_t seed = 0;
    UT rand_n = 0;

    int32_t get_loop_increment(int32_t range_min, int32_t range_max)
    {
#if defined(TEST_MIN_ITERATIONS)
        return (range_max - range_min)/TEST_MIN_ITERATIONS;
#else
        UNUSED(range_min);
        UNUSED(range_max);
        return 1;
#endif
    }
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
            exit(1);
        }
    }

    template <typename VecType, Branching ALGO>
    void test_vec(const T *numers, T denom, const divider<T, ALGO> &div) {
        // Align memory to 64 byte boundary for AVX512
        char mem[16 * sizeof(T) + 64];
        size_t offset = 64 - (size_t)&mem % 64;
        T *results = (T *)&mem[offset];

        size_t iters = 64 / sizeof(VecType);
        size_t size = sizeof(VecType) / sizeof(T);

        for (size_t j = 0; j < iters; j++, numers += size) {
            VecType x = *((const VecType *)numers);
            VecType resultVector = x / div;
            results = (T *)&resultVector;

            for (size_t i = 0; i < size; i++) {
                T numer = numers[i];
                T result = results[i];
                T expect = numer / denom;

                if (result != expect) {
                    PRINT_ERROR(F("Vector failure for: "));
                    PRINT_ERROR(testcase_name(ALGO));
                    PRINT_ERROR(F(": "));
                    PRINT_ERROR(numer);
                    PRINT_ERROR(F(" / "));
                    PRINT_ERROR(denom);
                    PRINT_ERROR(F(" = "));
                    PRINT_ERROR(expect );
                    PRINT_ERROR(F(", but got "));
                    PRINT_ERROR(result);
                    PRINT_ERROR(F("\n"));
                    exit(1);
                } else {
    #if 0
                        std::cout << "vec" << (CHAR_BIT * sizeof(VecType)) << " success for: " << numer << " / " << denom << " = " << result << std::endl;
    #endif
                }
            }
        }
    }

    template <Branching ALGO>
    void test_many(T denom) {
        // Don't try dividing by 1 with unsigned branchfree
        if (ALGO == BRANCHFREE && std::is_unsigned<T>::value && denom == 1) {
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
            exit(1);
        }

        T min = (limits::min)();
        T max = (limits::max)();

        static const T edgeCases[] = {
          0,
          (T)(1),
          (T)(2),
          (T)(3),
          (T)(4),
          (T)(5),
          (T)(6),
          (T)(7),
          (T)(8),
          (T)(9),
          (T)(10),
          (T)(11),
          (T)(12),
          (T)(13),
          (T)(14),
          (T)(15),
          (T)(16),
          (T)(17),
          (T)(18),
          (T)(19),
          (T)(20),
          (T)(21),
          (T)(22),
          (T)(23),
          (T)(24),
          (T)(25),
          (T)(26),
          (T)(27),
          (T)(28),
          (T)(29),
          (T)(30),
          (T)(31),
          (T)(32),
          (T)(33),
          (T)(34),
          (T)(35),
          (T)(36),
          (T)(37),
          (T)(38),
          (T)(39),
          (T)(40),
          (T)(41),
          (T)(42),
          (T)(43),
          (T)(44),
          (T)(45),
          (T)(46),
          (T)(47),
          (T)(48),
          (T)(49),
          (T)(123),
          (T)(1232),
          (T)(36847),
          (T)(506838),
          (T)(3000003),
          (T)(70000007),
          (T)(max),
          (T)(max - 1),
          (T)(max - 2),
          (T)(max - 3),
          (T)(max - 4),
          (T)(max - 5),
          (T)(max - 3213),
          (T)(max - 2453242),
          (T)(max - 432234231),
          (T)(min),
          (T)(min + 1),
          (T)(min + 2),
          (T)(min + 3),
          (T)(min + 4),
          (T)(min + 5),
          (T)(min + 3213),
          (T)(min + 2453242),
          (T)(min + 432234231),
          (T)(max / 2),
          (T)(max / 2 + 1),
          (T)(max / 2 - 1),
          (T)(max / 3),
          (T)(max / 3 + 1),
          (T)(max / 3 - 1),
          (T)(max / 4),
          (T)(max / 4 + 1),
          (T)(max / 4 - 1),
          (T)(min / 2),
          (T)(min / 2 + 1),
          (T)(min / 2 - 1),
          (T)(min / 3),
          (T)(min / 3 + 1),
          (T)(min / 3 - 1),
          (T)(min / 4),
          (T)(max / 4 + 1),
          (T)(min / 4 - 1)
        };

        for (T numerator : edgeCases) {
            test_one(numerator, denom, the_divider);
        }

        // balance signed & unsigned testing
        int32_t small_stop = (limits::is_signed) ? (int32_t)1 << 14 : (uint32_t)1 << 16;

        // test small numerators < 2^16
        int32_t increment = get_loop_increment(0, small_stop);
        for (int32_t i = 0; i < small_stop; i+=increment) {
            test_one(i, denom, the_divider);

            if (limits::is_signed) {
                test_one(-i, denom, the_divider);
            }
        }

        // test power of 2 numerators: 2^i-1, 2^i, 2^i+1
        for (int i = 1; i < limits::digits; i++) {
            for (int j = -1; j <= 1; j++) {
                T numerator = ((T)1 << i) + j;
                test_one(numerator, denom, the_divider);

                if (limits::is_signed) {
                    test_one(-numerator, denom, the_divider);
                }
            }
        }

        // test all bits set:
        // 11111111, 11111110, 11111100, ...
        for (UT bits = (UT)~0ull; bits != 0; bits <<= 1) {
            test_one((T)bits, denom, the_divider);
        }

        // Align memory to 64 byte boundary for AVX512
        char mem[16 * sizeof(T) + 64];
        size_t offset = 64 - (size_t)&mem % 64;
        T *numers = (T *)&mem[offset];

        // test random numerators
        increment = get_loop_increment(0, 10000);
        for (size_t i = 0; i < 10000; i += increment) {
            for (size_t j = 0; j < 16; j++) {
                numers[j] = get_random();
            }
            for (size_t j = 0; j < 16; j++) {
                test_one(numers[j], denom, the_divider);
            }
#ifdef LIBDIVIDE_SSE2
            test_vec<__m128i>(numers, denom, the_divider);
#endif
#ifdef LIBDIVIDE_AVX2
            test_vec<__m256i>(numers, denom, the_divider);
#endif
#ifdef LIBDIVIDE_AVX512
            test_vec<__m512i>(numers, denom, the_divider);
#endif
#ifdef LIBDIVIDE_NEON
            test_vec<typename NeonVecFor<T>::type>(numers, denom, the_divider);
#endif
        }
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

    void test_all_algorithms(T denom) {
        PRINT_PROGRESS_MSG(F("Testing deom "));
        PRINT_PROGRESS_MSG(denom);
        PRINT_PROGRESS_MSG(F("\n"));
        test_many<BRANCHFULL>(denom);
        test_many<BRANCHFREE>(denom);        
    }

public:

    DivideTest() {
        seed = randomSeed();
        rand_n = (UT)randomSeed();
    }

    void run() {
        // Test small values
        int32_t increment = get_loop_increment(0, 1024);
        for (int denom = 1; denom < 1024; denom+=increment) {
            test_all_algorithms(denom);

            if (limits::is_signed) {
                test_all_algorithms(-denom);
            }
        }

        if (limits::is_signed) {
            PRINT_PROGRESS_MSG(F("Testing minimum\n"));
            test_all_algorithms((limits::min)());
        }

        PRINT_PROGRESS_MSG(F("Testing maximum\n"));
        test_all_algorithms((limits::max)());

        // test power of 2 denoms: 2^i-1, 2^i, 2^i+1
        PRINT_PROGRESS_MSG(F("Testing powers of 2\n"));
        for (int i = 1; i < limits::digits; i++) {
            for (int j = -1; j <= 1; j++) {
                T denom = ((T)1 << i) + j;

                test_all_algorithms(denom);

                if (limits::is_signed) {
                    test_all_algorithms(-denom);
                }
            }
        }

        // test all bits set:
        // 11111111, 11111110, 11111100, ...
        PRINT_PROGRESS_MSG(F("Testing all bits set\n"));
        for (UT bits = (UT)~0ull; bits != 0; bits <<= 1) {
            test_all_algorithms((T)bits);
        }

        // Test random denominators
        PRINT_PROGRESS_MSG(F("Test random denominators\n"));        
        increment = get_loop_increment(0, 10000);
        for (int i = 0; i < 10000; i+=increment) {
            T denom = random_denominator();

            test_all_algorithms(denom);
        }
    }
};

template <typename T>
void run_test() {
    PRINT_INFO(F("Testing "));
    PRINT_INFO(type_name<T>::get_name());
    PRINT_INFO(F("\n"));
    DivideTest<T> dt;
    dt.run();
}