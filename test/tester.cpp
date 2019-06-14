// Usage: tester [OPTIONS]
//
// You can pass the tester program one or more of the following options:
// u32, s32, u64, s64 or run it without arguments to test all four.
// The tester is multithreaded so it can test multiple cases simultaneously.
// The tester will verify the correctness of libdivide via a set of
// randomly chosen denominators, by comparing the result of libdivide's
// division to hardware division. It may take a long time to run, but it
// will output as soon as it finds a discrepancy.

#include "libdivide.h"

#include <cstdlib>
#include <future>
#include <iostream>
#include <limits>
#include <random>
#include <string.h>
#include <string>
#include <sstream>

using namespace std;
using namespace libdivide;

template<typename T>
class DivideTest
{
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
            if (seed % 2)
                return -(T) rand_n;
        }

        return (T) rand_n;
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

    template<int ALGO>
    void test_one(T numer, T denom, const divider<T, ALGO>& the_divider) {
        // Don't crash with INT_MIN / -1
        // INT_MIN / -1 is undefined behavior in C/C++
        if (limits::is_signed && 
            numer == limits::min() && 
            denom == T(-1)) {
            return;
        }

        T expect = numer / denom;
        T actual = numer / the_divider;

        if (actual != expect) {
            cerr << "Failure for " << testcase_name(ALGO) << ": " <<  numer << " / " << denom << " expected " << expect << " actual " << actual << endl;
            exit(1);
        }
    }

#if defined(LIBDIVIDE_AVX512) || \
    defined(LIBDIVIDE_AVX2) || \
    defined(LIBDIVIDE_SSE2)   

#if defined(LIBDIVIDE_AVX512)
    #define VECTOR_TYPE __m512i
    #define VECTOR_LOAD _mm512_loadu_si512
#elif defined(LIBDIVIDE_AVX2)
    #define VECTOR_TYPE __m256i
    #define VECTOR_LOAD _mm256_loadu_si256
#elif defined(LIBDIVIDE_SSE2)
    #define VECTOR_TYPE __m128i
    #define VECTOR_LOAD _mm_loadu_si128
#endif

    template<int ALGO>
    void test_16(const T *numers, T denom, const divider<T, ALGO> & the_divider) {
        // Align memory to 64 byte boundary for AVX512
        char mem[16 * sizeof(T) + 64];
        size_t offset = 64 - (size_t)&mem % 64;
        T* results = (T*) &mem[offset];

        size_t iters = 64 / sizeof(VECTOR_TYPE);
        size_t size = sizeof(VECTOR_TYPE) / sizeof(T);

        for (size_t j = 0; j < iters; j++, numers += size) {
            VECTOR_TYPE x = VECTOR_LOAD((const VECTOR_TYPE*) numers);
            VECTOR_TYPE resultVector = x / the_divider;
            results = (T*) &resultVector;

            for (size_t i = 0; i < size; i++) {
                T numer = numers[i];
                T actual = results[i];
                T expect = numer / denom;
                if (actual != expect) {
                    ostringstream oss;
                    oss << "Vector failure for: " << testcase_name(ALGO) << ": " <<  numer << " / " << denom << " expected " << expect << " actual " << actual << endl;
                    cerr << oss.str();
                    exit(1);
                }
                else {
                    #if 0
                        ostringstream oss;
                        oss << "Vector success for: " << numer << " / " << denom << " = " << actual << " (" << i << ")" << endl;
                        cout << oss.str();
                    #endif
                }
            }
        }
    }
#endif

    template<int ALGO>
    void test_many(T denom) {
        // Don't try dividing by 1 with unsigned branchfree
        if (ALGO == BRANCHFREE && 
            std::is_unsigned<T>::value &&
            denom == 1) {
            return;
        }

        const divider<T, ALGO> the_divider = divider<T, ALGO>(denom);
        T recovered = the_divider.recover_divisor(); 
        if (recovered != denom) {
            cerr << "Failed to recover divisor for " << testcase_name(ALGO) << ": "<< denom << ", but got " << recovered << endl;
            exit(1);
        }

        T min = limits::min();
        T max = limits::max();

        T edgeCases[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
                          10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
                          20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
                          30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
                          40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
                          123, 1233, 36847, 506831, 3000003, 70000007,
                          max, max-1, max-2, max-3, max-4, max-5, max-3213, max-2453242, max-432234231,
                          min, min+1, min+2, min+3, min+4, min+5, min+3213, min+2453242, min+432234231,
                          max/2, max/2+1, max/2-1, max/3, max/3+1, max/3-1, max/4, max/4+1, max/4-1,
                          min/2, min/2+1, min/2-1, min/3, min/3+1, min/3-1, min/4, max/4+1, min/4-1 };

        for (size_t i = 0; i < sizeof(edgeCases) / sizeof(*edgeCases); i++) {
            test_one(edgeCases[i], denom, the_divider);
        }

        // balance signed & unsigned testing
        int small_stop = (limits::is_signed) ? 1 << 15 : 1 << 16;

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
        for (UT bits = (UT) ~0ull; bits != 0; bits <<= 1) {
            test_one((T) bits, denom, the_divider);
        }

        // Align memory to 64 byte boundary for AVX512
        char mem[16 * sizeof(T) + 64];
        size_t offset = 64 - (size_t)&mem % 64;
        T* numers = (T*) &mem[offset];

        // test random numerators
        for (size_t i = 0; i < 10000; i += 16) {
            for (size_t j = 0; j < 16; j++) {
                numers[j] = get_random();
            }
            for (size_t j = 0; j < 16; j++) {
                test_one(numers[j], denom, the_divider);
            }
#if defined(LIBDIVIDE_AVX512) || \
    defined(LIBDIVIDE_AVX2) || \
    defined(LIBDIVIDE_SSE2)   
            test_16(numers, denom, the_divider);
#endif
        }
    }

public:
    DivideTest(const std::string &n) :
        name(n)
    {
        std::random_device randomDevice;
        std::mt19937 randGen(randomDevice());
        std::uniform_int_distribution<uint32_t> randDist(1, numeric_limits<uint32_t>::max());
        seed = randDist(randGen);
        rand_n = (UT) randDist(randGen);
    }

    void run(void) {
        // Test small values
        for (T denom = 1; denom < 1024; denom++) {
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
        for (UT bits = (UT) ~0ull; bits != 0; bits <<= 1) {
            test_many<BRANCHFULL>((T) bits);
            test_many<BRANCHFREE>((T) bits);
        }

        // Test randomish values
        for (unsigned i=0; i < 10000; i++) {
            T denom = random_denominator();
            test_many<BRANCHFULL>(denom);
            test_many<BRANCHFREE>(denom);
        }
    }
};

int sRunS32 = 0;
int sRunU32 = 0;
int sRunS64 = 0;
int sRunU64 = 0;

void run_test(int idx) {
    switch (idx) {
        case 0:
        {
            if (!sRunS32) break;
            puts("Starting int32_t");
            DivideTest<int32_t> dt("s32");
            dt.run();
            break;
        }   
        case 1:
        {
            if (!sRunU32) break;
            puts("Starting uint32_t");
            DivideTest<uint32_t> dt("u32");
            dt.run();
            break;
        }
        case 2:
        {
            if (!sRunS64) break;
            puts("Starting sint64_t");
            DivideTest<int64_t> dt("s64");
            dt.run();
            break;
        }
        case 3:
        {
            if (!sRunU64) break;
            puts("Starting uint64_t");
            DivideTest<uint64_t> dt("u64");
            dt.run();
            break;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        /* Test all */
        sRunU32 = sRunU64 = sRunS32 = sRunS64 = 1;
    }
    else {
        for (int i = 1; i < argc; i++) {
            if (!strcmp(argv[i], "u32")) sRunU32 = 1;
            else if (!strcmp(argv[i], "u64")) sRunU64 = 1;
            else if (!strcmp(argv[i], "s32")) sRunS32 = 1;
            else if (!strcmp(argv[i], "s64")) sRunS64 = 1;
            else {
                cout << "Usage: tester [OPTIONS]\n"
                       "\n"
                       "You can pass the tester program one or more of the following options:\n"
                       "u32, s32, u64, s64 or run it without arguments to test all four.\n"
                       "The tester is multithreaded so it can test multiple cases simultaneously.\n"
                       "The tester will verify the correctness of libdivide via a set of\n"
                       "randomly chosen denominators, by comparing the result of libdivide's\n"
                       "division to hardware division. It may take a long time to run, but it\n"
                       "will output as soon as it finds a discrepancy." << endl;
                exit(1);
            }
        }
    }

    vector<future<void>> futures;
    futures.reserve(4);

    // Start 4 threads
    for (int test_id = 0; test_id < 4; test_id++) {
        futures.emplace_back(async(launch::async, run_test, test_id));
    }

    // Wait until threads finish
    for (auto &f : futures) {
        f.get();
    }

    cout << "\nAll tests passed successfully!" << endl;
    return 0;
}
