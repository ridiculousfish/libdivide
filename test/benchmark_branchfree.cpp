// Usage: benchmark_branchfree [u32] [u64] [s32] [s64] [branchfree] [branchfull] [sys|system]
//
// The branchfree benchmark iterates over an array of dividers and computes
// divisions. This is the use case where the branchfree divider generally
// shines and where the default branchfull divider performs poorly because
// the CPU is not able to correctly predict the branches of the many different
// dividers.
//

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <typeinfo>
#include <vector>
#if defined(__AVR__)
#include "avr_type_helpers.h"
#else
#include <type_traits>
#endif

#include "libdivide.h"
#include "type_mappings.h"

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
std::vector<divider_type> get_primes(T max) {
    size_t n = (size_t)max;
    std::vector<char> sieve(n + 1, true);

    for (size_t i = 2; i * i <= n; i++)
        if (sieve[i])
            for (size_t j = i * i; j <= n; j += i) sieve[j] = false;

    std::vector<divider_type> primes;
    for (size_t i = 2; i <= n; i++)
        if (sieve[i]) primes.push_back((T)i);

    return primes;
}

// Here we iterate over an array of dividers and compute divisions.
// libdivide's branchfull divider will not perform well as the
// CPU will not be able to correctly predict the branches.
// The branchfree divider is perfectly suited for this use case
// and will perform much better.
//
template <typename N, typename T>
NOINLINE size_t sum_dividers(N numerator, const T& dividers) {
    size_t sum = 0;

    for (const auto& divider : dividers) sum += (size_t)(numerator / divider);

    return sum;
}

struct result_t {
    double duration;
    size_t sum;
};

template <typename T, typename D>
NOINLINE result_t benchmark_sum_dividers(const D& dividers, size_t iters) {
    auto t1 = std::chrono::system_clock::now();
    size_t sum = 0;

    for (; iters > 0; iters--) {
        // Unsigned branchfree divider cannot be 1
        T numerator = std::max((T)2, (T)iters);
        sum += sum_dividers(numerator, dividers);
    }

    auto t2 = std::chrono::system_clock::now();
    std::chrono::duration<double> seconds = t2 - t1;
    return result_t{seconds.count(), sum};
}

enum {
    TEST_U16 = 1 << 0,
    TEST_U32 = 1 << 1,
    TEST_U64 = 1 << 2,
    TEST_S16 = 1 << 3,
    TEST_S32 = 1 << 4,
    TEST_S64 = 1 << 5,
    TEST_ALL_TYPES = (TEST_U16 | TEST_U32 | TEST_U64 | TEST_S16 | TEST_S32 | TEST_S64),
    TEST_SYSTEM = 1 << 6,
    TEST_BRANCHFREE = 1 << 7,
    TEST_BRANCHFULL = 1 << 8,
    TEST_ALL_ALGOS = (TEST_SYSTEM | TEST_BRANCHFREE | TEST_BRANCHFULL),
};

using tasks_t = unsigned int;

template <typename T>
void benchmark(tasks_t tasks, size_t max, size_t iters) {
    std::cout << "----- " << type_tag<T>::get_tag() << " -----" << std::endl;

    bool test_system = !!(tasks & TEST_SYSTEM);
    bool test_branchfull = !!(tasks & TEST_BRANCHFULL);
    bool test_branchfree = !!(tasks & TEST_BRANCHFREE);

    result_t sys = {0, 0};
    result_t branchfull = {0, 0};
    result_t branchfree = {0, 0};

    size_t st_max = std::min(max, (size_t)std::numeric_limits<T>::max());
    iters = iters * (max / st_max);
    T t_max = (T)st_max;
    if (test_system) {
        using divider_type = T;
        auto dividers = get_primes<divider_type>(t_max);
        sys = benchmark_sum_dividers<T>(dividers, iters);
        std::cout << '.' << std::flush;
    }

    if (test_branchfull) {
        using divider_type = libdivide::divider<T>;
        auto dividers = get_primes<divider_type>(t_max);
        branchfull = benchmark_sum_dividers<T>(dividers, iters);
        std::cout << '.' << std::flush;
    }

    if (test_branchfree) {
        using divider_type = libdivide::branchfree_divider<T>;
        auto dividers = get_primes<divider_type>(t_max);
        branchfree = benchmark_sum_dividers<T>(dividers, iters);
        std::cout << '.' << std::endl;
    }

    if (test_system && test_branchfull && branchfull.sum != sys.sum) {
        std::cerr << "Error: branchfull_divider<" << type_tag<T>::get_tag() << "> sum: " << branchfull.sum
                  << ", but system sum: " << sys.sum << std::endl;
        std::exit(1);
    }

    if (test_system && test_branchfree && branchfree.sum != sys.sum) {
        std::cerr << "Error: branchfree_divider<" << type_tag<T>::get_tag() << "> sum: " << branchfree.sum
                  << ", but system sum: " << sys.sum << std::endl;
        std::exit(1);
    }

    if (test_system) std::cout << "    system: " << sys.duration << " seconds" << std::endl;
    if (test_branchfull)
        std::cout << "branchfull: " << branchfull.duration << " seconds" << std::endl;
    if (test_branchfree)
        std::cout << "branchfree: " << branchfree.duration << " seconds" << std::endl;

    std::cout << std::endl;
}

void usage() {
    std::cout << "Usage: benchmark_branchfree [uu16] [u32] [u64] [s16] [s32] [s64] [branchfree] "
                 "[branchfull] "
                 "[sys|system]\n"
                 "\n"
                 "The branchfree benchmark iterates over an array of dividers and computes\n"
                 "divisions. This is the use case where the branchfree divider generally\n"
                 "shines and where the default branchfull divider performs poorly because\n"
                 "the CPU is not able to correctly predict the branches of the many different\n"
                 "dividers."
              << std::endl;
}

int main(int argc, const char* argv[]) {
    tasks_t tasks = 0;

    for (int i = 1; i < argc; i++) {
        std::string arg(argv[i]);

        if (arg == type_tag<uint16_t>::get_tag()) {
            tasks |= TEST_U16;
        } else if (arg == type_tag<int16_t>::get_tag()) {
            tasks |= TEST_S16;
        } else if (arg == type_tag<uint32_t>::get_tag()) {
            tasks |= TEST_U32;
        } else if (arg == type_tag<int32_t>::get_tag()) {
            tasks |= TEST_S32;
        } else if (arg == type_tag<uint64_t>::get_tag()) {
            tasks |= TEST_U64;
        } else if (arg == type_tag<int64_t>::get_tag()) {
            tasks |= TEST_S64;
        } else if (arg == "branchfree") {
            tasks |= TEST_BRANCHFREE;
        } else if (arg == "branchfull") {
            tasks |= TEST_BRANCHFULL;
        } else if (arg == "sys" || arg == "system") {
            tasks |= TEST_SYSTEM;
        } else {
            usage();
            return 1;
        }
    }

    // Set default tasks
    if (!(tasks & TEST_ALL_TYPES)) {
        tasks |= TEST_ALL_TYPES;
    }
    if (!(tasks & TEST_ALL_ALGOS)) {
        tasks |= TEST_ALL_ALGOS;
    }

    size_t iters = 3000;
    size_t max_divider = 1 << 22;

    if (tasks & TEST_U16) {
        benchmark<uint16_t>(tasks, max_divider, iters);
    }

    if (tasks & TEST_S16) {
        benchmark<int16_t>(tasks, max_divider, iters);
    }

    if (tasks & TEST_U32) {
        benchmark<uint32_t>(tasks, max_divider, iters);
    }

    if (tasks & TEST_S32) {
        benchmark<int32_t>(tasks, max_divider, iters);
    }

    if (tasks & TEST_U64) {
        benchmark<uint64_t>(tasks, max_divider, iters);
    }

    if (tasks & TEST_S64) {
        benchmark<int64_t>(tasks, max_divider, iters);
    }

    std::cout << "All tests passed successfully!" << std::endl;

    return 0;
}
