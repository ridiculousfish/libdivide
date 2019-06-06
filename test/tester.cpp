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

#include <limits.h>
#include <limits>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <typeinfo>
#include <limits>
#include <string.h>
#include <string>
#include <sstream>

#if defined(_WIN32) || defined(WIN32)
    // Windows makes you do a lot to stop it from "helping"
    #if !defined(NOMINMAX)
        #define NOMINMAX
    #endif
    #define WIN32_LEAN_AND_MEAN
    #define VC_EXTRALEAN
    #include <windows.h>
    #define LIBDIVIDE_WINDOWS
#else
    // Linux or Mac OS X or other Unix
    #include <pthread.h>
#endif

using namespace std;
using namespace libdivide;

#define SEED 2147483563

class DivideTest_PRNG {
public:
    DivideTest_PRNG() : seed(SEED) { }
        
protected:
    uint32_t seed;
    uint32_t next_random(void) {
        seed = seed * 1664525 + 1013904223U;
        return seed;
    }
    
};

template<typename T>
class DivideTest : private DivideTest_PRNG {

private:
    
    std::string name;
    
    typedef std::numeric_limits<T> limits;
    
    uint32_t base_random(void) {
        return this->next_random();
    }

    T random_denominator(void) {
        T result;
        if (sizeof(T) == 4) {
            do {
                result = base_random();
            } while (result == 0);
            return result;
        }
        else {
            do {
                uint32_t little = base_random(), big = base_random();
                result = (T)(little + ((uint64_t)big << 32));
            } while (result == 0);
        }
        return result;
    }
    
    std::string testcase_name(int algo) const {
        std::string result = this->name;
        if (algo == BRANCHFREE) {
            result += " (branchfree)";
        }
        return result;
    }
    
    void test_unswitching(T, T, const divider<T, BRANCHFREE> &) {
        // No unswitching in branchfree
    }

    void test_unswitching(T numer, T denom, const divider<T, BRANCHFULL> & the_divider) {
        T expect = numer / denom;
        T actual2 = -1;
        switch (the_divider.get_algorithm()) {
            case 0: actual2 = numer / unswitch<0>(the_divider); break;
            case 1: actual2 = numer / unswitch<1>(the_divider); break;
            case 2: actual2 = numer / unswitch<2>(the_divider); break;
            case 3: actual2 = numer / unswitch<3>(the_divider); break;
            case 4: actual2 = numer / unswitch<4>(the_divider); break;
            default:
                cout << "Unexpected algorithm" << the_divider.get_algorithm() << endl;
                while (1) ;
                break;
        }
        if (actual2 != expect) {
            cerr << "Unswitched failure for " << testcase_name(BRANCHFULL) << ": " <<  numer << " / " << denom << " expected " << expect << " actual " << actual2 <<  " algo " << the_divider.get_algorithm() << endl;
            exit(1);
        }
    }
    
    template<int ALGO>
    void test_one(T numer, T denom, const divider<T, ALGO> & the_divider) {
        // Don't crash with INT_MIN / -1
        if (limits::is_signed && numer == limits::min() && denom == T(-1)) {
            return;
        }
        
        T expect = numer / denom;
        T actual1 = numer / the_divider;
        if (actual1 != expect) {
            cerr << "Failure for " << testcase_name(ALGO) << ": " <<  numer << " / " << denom << " expected " << expect << " actual " << actual1 << endl;
            exit(1);
        }
        test_unswitching(numer, denom, the_divider);
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
        // Don't try dividing by +/- 1 with branchfree
        if (ALGO == BRANCHFREE && (denom == 1 || (limits::is_signed && denom == T(-1)))) {
            return;
        }

        const divider<T, ALGO> the_divider = divider<T, ALGO>(denom);
        T recovered = the_divider.recover_divisor(); 
        if (recovered != denom) {
            cerr << "Failed to recover divisor for " << testcase_name(ALGO) << ": "<< denom << ", but got " << recovered << endl;
            exit(1);
        }

        // Align memory to 64 byte boundary for AVX512
        char mem[16 * sizeof(T) + 64];
        size_t offset = 64 - (size_t)&mem % 64;
        T* numers = (T*) &mem[offset];

        for (size_t i = 0; i < 100000 / 16; i++) {
            for (size_t j = 0; j < 16; j++) {
                numers[j] = (T) this->next_random();
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
        const T min = limits::min(), max = limits::max();
        const T wellKnownNumers[] = {0, max, max-1, max/2, max/2 - 1, min, min/2, min/4, 1, 2, 3, 4, 5, 6, 7, 8, 10, 36847, 50683, SHRT_MAX};
        for (size_t j =0; j < sizeof wellKnownNumers / sizeof *wellKnownNumers; j++) {
            if (wellKnownNumers[j] == 0 && j != 0)
                continue;
            test_one(wellKnownNumers[j], denom, the_divider);
        }
        T powerOf2Numer = (limits::max()>>1)+1;
        while (powerOf2Numer != 0) {
            test_one(powerOf2Numer, denom, the_divider);
            powerOf2Numer /= 2;
        }
    }
    
public:
    
    DivideTest(const std::string &n) : name(n) { }
    
    void run(void) {
        // Test small values
        for (T denom = 1; denom < 257; denom++) {
            // powers of 2 get tested later
            if ((denom & (denom - 1)) == 0) continue;
            test_many<BRANCHFULL>(denom);
            test_many<BRANCHFREE>(denom);
            if (limits::is_signed) {
                test_many<BRANCHFULL>(-denom);
                test_many<BRANCHFREE>(-denom);
            }
        }
        
        
        /* Test key values */
        const T keyValues[] = {T((1<<15)+1), T((1<<31)+1), T((1LL<<63)+1)};
        for (size_t i=0; i < sizeof keyValues / sizeof *keyValues; i++) {
            T denom = keyValues[i];
            test_many<BRANCHFULL>(denom);
            test_many<BRANCHFREE>(denom);
            if (limits::is_signed) {
                test_many<BRANCHFULL>(-denom);
                test_many<BRANCHFREE>(-denom);
            }
        }
        
        // Test randomish values
        for (unsigned i=0; i < 10000; i++) {
            T denom = random_denominator();
            test_many<BRANCHFULL>(denom);
            test_many<BRANCHFREE>(denom);
            //cout << typeid(T).name() << "\t\t" << i << " / " << 100000 << endl;
        }
        
        /* Test powers of 2, both positive and negative. Careful to do no signed left shift of negative values. */
        T posPowOf2 = (limits::max() >> 1) + 1;
        while (posPowOf2 != 0) {
            test_many<BRANCHFULL>(posPowOf2);
            test_many<BRANCHFREE>(posPowOf2);
            posPowOf2 /= 2;
        }
        T negPowOf2 = limits::min(); // may be 0 already
        while (negPowOf2 != 0) {
            test_many<BRANCHFULL>(negPowOf2);
            test_many<BRANCHFREE>(negPowOf2);
            negPowOf2 /= 2; // assumes truncation towards 0
        }
    }
};

int sRunS32 = 0;
int sRunU32 = 0;
int sRunS64 = 0;
int sRunU64 = 0;

static void *perform_test(void *ptr) {
    intptr_t idx = (intptr_t)ptr;
    switch (idx) {
        case 0:
        {
            if (! sRunS32) break;
            puts("Starting int32_t");
            DivideTest<int32_t> dt("s32");
            dt.run();
        }
            break;
            
        case 1:
        {
            if (! sRunU32) break;
            puts("Starting uint32_t");
            DivideTest<uint32_t> dt("u32");
            dt.run();
        }
            break;
            
        case 2:
        {
            if (! sRunS64) break;
            puts("Starting sint64_t");
            DivideTest<int64_t> dt("s64");
            dt.run();
        }
            break;
            
        case 3:
        {
            if (! sRunU64) break;
            puts("Starting uint64_t");
            DivideTest<uint64_t> dt("u64");
            dt.run();
        }
            break;
    }
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc == 1) {
        /* Test all */
        sRunU32 = sRunU64 = sRunS32 = sRunS64 = 1;
    }
    else {
        for (int i = 1; i < argc; i++) {
            if (! strcmp(argv[i], "u32")) sRunU32 = 1;
            else if (! strcmp(argv[i], "u64")) sRunU64 = 1;
            else if (! strcmp(argv[i], "s32")) sRunS32 = 1;
            else if (! strcmp(argv[i], "s64")) sRunS64 = 1;
            else {
                printf("Usage: tester [OPTIONS]\n"
                       "\n"
                       "You can pass the tester program one or more of the following options:\n"
                       "u32, s32, u64, s64 or run it without arguments to test all four.\n"
                       "The tester is multithreaded so it can test multiple cases simultaneously.\n"
                       "The tester will verify the correctness of libdivide via a set of\n"
                       "randomly chosen denominators, by comparing the result of libdivide's\n"
                       "division to hardware division. It may take a long time to run, but it\n"
                       "will output as soon as it finds a discrepancy.\n");
                exit(1);
            }
        }
    }

/* We could use dispatch, but we prefer to use pthreads because dispatch won't run all four tests at once on a two core machine */
#if defined(DISPATCH_API_VERSION)
    dispatch_apply(4, dispatch_get_global_queue(0, 0), ^(size_t x){
        perform_test((void *)(intptr_t)x);
    });
#elif defined(LIBDIVIDE_WINDOWS)
	HANDLE threadArray[4];
	intptr_t i;
	for (i=0; i < 4; i++) {
		threadArray[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)perform_test, (void *)i, 0, NULL);
	}
	WaitForMultipleObjects(4, threadArray, TRUE, INFINITE);
#else
    pthread_t threads[4];
    intptr_t i;
    for (i=0; i < 4; i++) {
        int err = pthread_create(&threads[i], NULL, perform_test, (void *)i);
        if (err) {
            fprintf(stderr, "pthread_create() failed\n");
            exit(EXIT_FAILURE);
        }
    }
    for (i=0; i < 4; i++) {
        void *dummy;
        pthread_join(threads[i], &dummy);
    }
#endif

    printf("\nAll tests passed successfully!\n");
    return 0;
}
