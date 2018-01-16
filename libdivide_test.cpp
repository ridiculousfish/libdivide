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

#if defined(LIBDIVIDE_USE_SSE2)
#include <emmintrin.h>
#endif

#if defined(_WIN32) || defined(WIN32)
/* Windows makes you do a lot to stop it from "helping" */
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <windows.h>
#define LIBDIVIDE_WINDOWS
#else
/* Linux or Mac OS X or other Unix */
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
            cout << "Unswitched failure for " << testcase_name(BRANCHFULL) << ": " <<  numer << " / " << denom << " expected " << expect << " actual " << actual2 <<  " algo " << the_divider.get_algorithm() << endl;
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
            cout << "Failure for " << testcase_name(ALGO) << ": " <<  numer << " / " << denom << " expected " << expect << " actual " << actual1 << endl;
		}
        test_unswitching(numer, denom, the_divider);
    }

#if defined(LIBDIVIDE_USE_SSE2)
    template<int ALGO>
    void test_four(const T *numers, T denom, const divider<T, ALGO> & the_divider) {
		const size_t count = 16 / sizeof(T);
#if defined(LIBDIVIDE_VC)
		_declspec(align(16)) T results[count];
#else
		T __attribute__((aligned)) results[count];
#endif
		__m128i resultVector = _mm_loadu_si128((const __m128i *)numers) / the_divider;
		*(__m128i *)results = resultVector;
		size_t i;
		for (i = 0; i < count; i++) {
			T numer = numers[i];
			T actual = results[i];
			T expect = numer / denom;
			if (actual != expect) {
                cout << "Vector failure for " << testcase_name(ALGO) << ": " <<  numer << " / " << denom << " expected " << expect << " actual " << actual << endl;
			}
			else {
				//cout << "Vector success for " << numer << " / " << denom << " = " << actual << " (" << i << ")" << endl;
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
            cout << "Failed to recover divisor for " << testcase_name(ALGO) << ": "<< denom << ", but got " << recovered << endl;
        }
        
        size_t j;
        for (j=0; j < 100000 / 4; j++) {
            T numers[4] = {(T)this->next_random(), (T)this->next_random(), (T)this->next_random(), (T)this->next_random()};
            test_one(numers[0], denom, the_divider);
            test_one(numers[1], denom, the_divider);
            test_one(numers[2], denom, the_divider);
            test_one(numers[3], denom, the_divider);
#if defined(LIBDIVIDE_USE_SSE2)
            test_four(numers, denom, the_divider);
#endif
        }
        const T min = limits::min(), max = limits::max();
        const T wellKnownNumers[] = {0, max, max-1, max/2, max/2 - 1, min, min/2, min/4, 1, 2, 3, 4, 5, 6, 7, 8, 10, 36847, 50683, SHRT_MAX};
        for (j=0; j < sizeof wellKnownNumers / sizeof *wellKnownNumers; j++) {
            if (wellKnownNumers[j] == 0 && j != 0) continue;
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
        int i;
        for (i=1; i < argc; i++) {
            if (! strcmp(argv[i], "u32")) sRunU32 = 1;
            else if (! strcmp(argv[i], "u64")) sRunU64 = 1;
            else if (! strcmp(argv[i], "s32")) sRunS32 = 1;
            else if (! strcmp(argv[i], "s64")) sRunS64 = 1;
            else printf("Unknown test '%s'\n", argv[i]), exit(0);
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
    return 0;
}
