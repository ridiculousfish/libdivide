// Usage: tester [OPTIONS]
//
// You can pass the tester program one or more of the following options:
// u32, s32, u64, s64 or run it without arguments to test all four.
// The tester is multithreaded so it can test multiple cases simultaneously.
// The tester will verify the correctness of libdivide via a set of
// randomly chosen denominators, by comparing the result of libdivide's
// division to hardware division. It may take a long time to run, but it
// will output as soon as it finds a discrepancy.

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

#include "libdivide.h"

using namespace libdivide;

template <typename T>
class DivideTest {
   private:
    using UT = typename std::make_unsigned<T>::type;
    using limits = std::numeric_limits<T>;
    std::string name;
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

    std::string testcase_name(int algo) const {
        std::string result = this->name;
        if (algo == BRANCHFREE) {
            result += " (branchfree)";
        }
        return result;
    }

    template <Branching ALGO>
    void test_one(T numer, T denom, const divider<T, ALGO> &the_divider) {
        // Don't crash with INT_MIN / -1
        // INT_MIN / -1 is undefined behavior in C/C++
        if (limits::is_signed && numer == limits::min() && denom == T(-1)) {
            return;
        }

        T expect = numer / denom;
        T result = numer / the_divider;

        if (result != expect) {
            std::cerr << "Failure for " << testcase_name(ALGO) << ": " << numer << " / " << denom
                      << " = " << expect << ", but got " << result << std::endl;
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
                    std::cerr << "Vector failure for: " << testcase_name(ALGO) << ": " << numer
                              << " / " << denom << " = " << expect << ", but got " << result
                              << std::endl;
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
            std::cerr << "Failed to recover divisor for " << testcase_name(ALGO) << ": " << denom
                      << ", but got " << recovered << std::endl;
            exit(1);
        }

        T min = limits::min();
        T max = limits::max();

        static const T edgeCases[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
            18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
            40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 123, 1232, 36847, 506838, 3000003, 70000007,
            max, max - 1, max - 2, max - 3, max - 4, max - 5, max - 3213, max - 2453242,
            max - 432234231, min, min + 1, min + 2, min + 3, min + 4, min + 5, min + 3213,
            min + 2453242, min + 432234231, max / 2, max / 2 + 1, max / 2 - 1, max / 3, max / 3 + 1,
            max / 3 - 1, max / 4, max / 4 + 1, max / 4 - 1, min / 2, min / 2 + 1, min / 2 - 1,
            min / 3, min / 3 + 1, min / 3 - 1, min / 4, max / 4 + 1, min / 4 - 1};

        for (T numerator : edgeCases) {
            test_one(numerator, denom, the_divider);
        }

        // balance signed & unsigned testing
        int small_stop = (limits::is_signed) ? 1 << 14 : 1 << 16;

        // test small numerators < 2^16
        for (int i = 0; i < small_stop; i++) {
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
        for (size_t i = 0; i < 10000; i += 16) {
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
        }
    }

   public:
    DivideTest(const std::string &n) : name(n) {
        std::random_device randomDevice;
        std::mt19937 randGen(randomDevice());
        std::uniform_int_distribution<uint32_t> randDist(1, std::numeric_limits<uint32_t>::max());
        seed = randDist(randGen);
        rand_n = (UT)randDist(randGen);
    }

    void run() {
        // Test small values
        for (int denom = 1; denom < 1024; denom++) {
            test_many<BRANCHFULL>(denom);
            test_many<BRANCHFREE>(denom);

            if (limits::is_signed) {
                test_many<BRANCHFULL>(-denom);
                test_many<BRANCHFREE>(-denom);
            }
        }

        if (limits::is_signed) {
            test_many<BRANCHFULL>(limits::min());
            test_many<BRANCHFREE>(limits::min());
        }

        test_many<BRANCHFULL>(limits::max());
        test_many<BRANCHFREE>(limits::max());

        // test power of 2 denoms: 2^i-1, 2^i, 2^i+1
        for (int i = 1; i < limits::digits; i++) {
            for (int j = -1; j <= 1; j++) {
                T denom = ((T)1 << i) + j;
                test_many<BRANCHFULL>(denom);
                test_many<BRANCHFREE>(denom);

                if (limits::is_signed) {
                    test_many<BRANCHFULL>(-denom);
                    test_many<BRANCHFREE>(-denom);
                }
            }
        }

        // test all bits set:
        // 11111111, 11111110, 11111100, ...
        for (UT bits = (UT)~0ull; bits != 0; bits <<= 1) {
            test_many<BRANCHFULL>((T)bits);
            test_many<BRANCHFREE>((T)bits);
        }

        // Test random denominators
        for (int i = 0; i < 10000; i++) {
            T denom = random_denominator();
            test_many<BRANCHFULL>(denom);
            test_many<BRANCHFREE>(denom);
        }
    }
};

enum TestType {
    type_s32,
    type_u32,
    type_s64,
    type_u64,
};

template <typename T>
void run_test(const char *name) {
    DivideTest<T> dt(name);
    dt.run();
}

int main(int argc, char *argv[]) {
    const TestType test_types[] = {type_s32, type_u32, type_s64, type_u64};
    bool default_do_test = (argc <= 1);
    std::vector<bool> do_tests(4, default_do_test);

    for (int i = 1; i < argc; i++) {
        const std::string arg(argv[i]);
        if (arg == "s32")
            do_tests[type_s32] = true;
        else if (arg == "u32")
            do_tests[type_u32] = true;
        else if (arg == "s64")
            do_tests[type_s64] = true;
        else if (arg == "u64")
            do_tests[type_u64] = true;
        else {
            std::cout
                << "Usage: tester [OPTIONS]\n"
                   "\n"
                   "You can pass the tester program one or more of the following options:\n"
                   "u32, s32, u64, s64 or run it without arguments to test all four.\n"
                   "The tester is multithreaded so it can test multiple cases simultaneously.\n"
                   "The tester will verify the correctness of libdivide via a set of\n"
                   "randomly chosen denominators, by comparing the result of libdivide's\n"
                   "division to hardware division. It may take a long time to run, but it\n"
                   "will output as soon as it finds a discrepancy."
                << std::endl;
            exit(1);
        }
    }

    std::string vecTypes = "";
#if defined(LIBDIVIDE_SSE2)
    vecTypes += "sse2 ";
#endif
#if defined(LIBDIVIDE_AVX2)
    vecTypes += "avx2 ";
#endif
#if defined(LIBDIVIDE_AVX512)
    vecTypes += "avx512 ";
#endif
    if (vecTypes.empty()) {
        vecTypes = "none ";
    }
    vecTypes.back() = '\n';  // trailing space
    std::cout << "Testing with SIMD ISAs: " << vecTypes;

    // Run tests in threads.
    std::vector<std::thread> test_threads;
    if (do_tests[type_s32]) {
        std::cout << "Testing int32_t\n";
        test_threads.emplace_back(run_test<int32_t>, "s32");
    }
    if (do_tests[type_u32]) {
        std::cout << "Testing uint32_t\n";
        test_threads.emplace_back(run_test<uint32_t>, "u32");
    }
    if (do_tests[type_s64]) {
        std::cout << "Testing int64_t\n";
        test_threads.emplace_back(run_test<int64_t>, "s64");
    }
    if (do_tests[type_u64]) {
        std::cout << "Testing uint64_t\n";
        test_threads.emplace_back(run_test<uint64_t>, "u64");
    }
    for (auto &t : test_threads) {
        t.join();
    }

    std::cout << "\nAll tests passed successfully!" << std::endl;
    return 0;
}
