// Usage: benchmark_branchfree [u32] [u64] [s32] [s64] [branchfree] [branchfull] [sys|system]
//
// The branchfree benchmark iterates over an array of dividers and computes
// divisions. This is the use case where the branchfree divider generally
// shines and where the default branchfull divider performs poorly because
// the CPU is not able to correctly predict the branches of the many different
// dividers.
//

#include <algorithm>
#include <iostream>
#include <chrono>
#include <vector>
#include <cstring>
#include <cstdlib>

#include "libdivide.h"

#if defined(__GNUC__)
    #define NOINLINE __attribute__((__noinline__))
#elif defined(_MSC_VER)
    #define NOINLINE __declspec(noinline)
#else
    #define NOINLINE
#endif

// Generate primes using the sieve of Eratosthenes.
// These primes will later be used as dividers in the benchmark.
template <typename divider_type, typename T>
std::vector<divider_type> get_primes(T max)
{
    uint64_t n = (uint64_t) max;
    std::vector<divider_type> primes;
    std::vector<char> sieve(n + 1, true);

    for (uint64_t i = 2; i * i <= n; i++)
        if (sieve[i])
            for (uint64_t j = i * i; j <= n; j += i)
                sieve[j] = false;

    for (uint64_t i = 2; i <= n; i++)
        if (sieve[i])
            primes.push_back((T) i);

    return primes;
}

// Here we iterate over an array of dividers and compute divisions.
// libdivide's branchfull divider will not perform well as the
// CPU will not be able to correctly predict the branches.
// The branchfree divider is perfectly suited for this use case
// and will perform much better.
//
template<typename N, typename T>
NOINLINE size_t sum_dividers(N numerator, const T& dividers)
{
    size_t sum = 0;

    for (const auto& divider: dividers)
        sum += numerator / divider;

    return sum;
}

struct result_t {
    double duration;
    size_t sum;
};

template<typename T, typename P>
NOINLINE result_t benchmark_sum_dividers(const P& dividers, size_t iters) {
    using namespace std::chrono;
    high_resolution_clock::time_point t1 = high_resolution_clock::now();
    size_t sum = 0;

    for (; iters > 0; iters--) {
        // branchfree dividers current don't support +-1
        T numerator = std::max((T) 2, (T) iters);
        sum += sum_dividers(numerator, dividers);
    }

    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
    return result_t{time_span.count(), sum};
}

enum {
    TEST_U32 = 1 << 0,
    TEST_U64 = 1 << 1,
    TEST_S32 = 1 << 2,
    TEST_S64 = 1 << 3,
    TEST_ALL_TYPES = (TEST_U32 | TEST_U64 | TEST_S32 | TEST_S64),
    TEST_SYSTEM = 1 << 4,
    TEST_BRANCHFREE = 1 << 5,
    TEST_BRANCHFULL = 1 << 6,
    TEST_ALL_ALGOS = (TEST_SYSTEM | TEST_BRANCHFREE | TEST_BRANCHFULL),
};

typedef unsigned int tasks_t;

template<typename T>
void  benchmark(tasks_t tasks, size_t max, size_t iters) {
    bool test_system = !! (tasks & TEST_SYSTEM);
    bool test_branchfull = !! (tasks & TEST_BRANCHFULL);
    bool test_branchfree = !! (tasks & TEST_BRANCHFREE);
    
    result_t sys = {0, 0};
    result_t branchfull = {0, 0};
    result_t branchfree = {0, 0};

    if (test_system) {
        using divider_type = T;
        auto dividers = get_primes<divider_type>((T) max);
        sys = benchmark_sum_dividers<T>(dividers, iters);
        std::cout << '.' << std::flush;
    }
    
    if (test_branchfull) {
        using divider_type = libdivide::divider<T>;
        auto dividers = get_primes<divider_type>((T) max);
        branchfull = benchmark_sum_dividers<T>(dividers, iters);
        std::cout << '.' << std::flush;
    }

    if (test_branchfree) {
        using divider_type = libdivide::branchfree_divider<T>;
        auto dividers = get_primes<divider_type>((T) max);
        branchfree = benchmark_sum_dividers<T>(dividers, iters);
        std::cout << '.' << std::endl;
    }

    if (test_system && test_branchfull && branchfull.sum != sys.sum) {
        std::cerr << "Disagreement in branchfull path for type " << typeid(T).name() << ": libdivide says " << branchfull.sum << " but system says " << sys.sum << std::endl;
        std::exit(1);
    }
    if (test_system && test_branchfree && branchfree.sum != sys.sum) {
        std::cerr << "Disagreement in branchfree path for type " << typeid(T).name() << ": libdivide says " << branchfree.sum << " but system says " << sys.sum << std::endl;
        std::exit(1);
    }

    if (test_system)
        std::cout << "    system: " << sys.duration << " seconds" << std::endl;
    if (test_branchfull)
        std::cout << "branchfull: " << branchfull.duration << " seconds" << std::endl;
    if (test_branchfree)
        std::cout << "branchfree: " << branchfree.duration << " seconds" << std::endl;

    std::cout << std::endl;
}

void usage() {
    std::cout << "Usage: benchmark_branchfree [u32] [u64] [s32] [s64] [branchfree] [branchfull] [sys|system]\n"
                 "\n"
                 "The branchfree benchmark iterates over an array of dividers and computes\n"
                 "divisions. This is the use case where the branchfree divider generally\n"
                 "shines and where the default branchfull divider performs poorly because\n"
                 "the CPU is not able to correctly predict the branches of the many different\n"
                 "dividers." << std::endl;
}

int main(int argc, const char *argv[]) {
    tasks_t tasks = 0;
    
    // parse argv
    for (int i = 1; i < argc; i++) {
        const char * arg = argv[i];
        if (! strcmp(arg, "u32")) {
            tasks |= TEST_U32;
        } else if (! strcmp(arg, "s32")) {
            tasks |= TEST_S32;
        } else if (! strcmp(arg, "u64")) {
            tasks |= TEST_U64;
        } else if (! strcmp(arg, "s64")) {
            tasks |= TEST_S64;
        } else if (! strcmp(arg, "branchfree")) {
            tasks |= TEST_BRANCHFREE;
        } else if (! strcmp(arg, "branchfull")) {
            tasks |= TEST_BRANCHFULL;
        } else if (! strcmp(arg, "sys") || ! strcmp(arg, "system")) {
            tasks |= TEST_SYSTEM;
        } else {
            usage();
            return 1;
        }
    }

    // Set default tasks
    if (! (tasks & TEST_ALL_TYPES)) {
        tasks |= TEST_ALL_TYPES;
    }
    if (! (tasks & TEST_ALL_ALGOS)) {
        tasks |= TEST_ALL_ALGOS;
    }

    size_t iters = 3000;
    size_t max_divider = 1 << 22;

    if (tasks & TEST_U32) {
        std::cout << "----- u32 -----" << std::endl;
        benchmark<uint32_t>(tasks, max_divider, iters);
    }
    
    if (tasks & TEST_S32) {
        std::cout << "----- s32 -----" << std::endl;
        benchmark<int32_t>(tasks, max_divider, iters);
    }
    
    if (tasks & TEST_U64) {
        std::cout << "----- u64 -----" << std::endl;
        benchmark<uint64_t>(tasks, max_divider, iters);
    }

    if (tasks & TEST_S64) {
        std::cout << "----- s64 -----" << std::endl;
        benchmark<int64_t>(tasks, max_divider, iters);
    }

    std::cout << "All tests passed successfully!" << std::endl;

    return 0;
}
