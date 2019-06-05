#include <algorithm>
#include <functional>
#include <iostream>
#include <chrono>
#include <deque>
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

template<typename T, int ALGO>
struct prime_divider_t {
    T value;
    libdivide::divider<T, ALGO> divider;
    
    prime_divider_t(T v) : value(v), divider(v) {}
};

template<typename T, int ALGO>
NOINLINE size_t count_primes_libdivide(T max)
{
    std::vector<prime_divider_t<T, ALGO> > primes;
    primes.push_back(2);
    for (T i=3; i < max; i+=2) {
        bool is_prime = true;
        for (const auto &prime : primes) {
            T quotient = i / prime.divider;
            T remainder = i - quotient * prime.value;
            if (remainder == 0) {
                is_prime = false;
                break;
            }
        }
        if (is_prime) {
            primes.push_back(i);
        }
    }
    return primes.size();
}

template<typename T>
NOINLINE size_t count_primes_system(T max)
{
    std::vector<T> primes;
    primes.push_back(2);
    for (T i=3; i < max; i+=2) {
        bool is_prime = true;
        for (const auto &prime : primes) {
            if (i % prime == 0) {
                is_prime = false;
                break;
            }
        }
        if (is_prime) {
            primes.push_back(i);
        }
    }
    return primes.size();
}


template<typename Ret, typename... Args>
std::pair<double, Ret> time_function(std::function<Ret(Args...)> func, Args... args) {
    using namespace std::chrono;
    high_resolution_clock::time_point t1 = high_resolution_clock::now();
    size_t result = func(args...);
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
    return std::make_pair(time_span.count(), result);
}

struct prime_calculation_result_t {
    double duration;
    size_t result;
};

template<typename T, size_t Func(T)>
prime_calculation_result_t measure_1_prime_calculation(T max, size_t iters) {
    double best = std::numeric_limits<double>::max();
    size_t result = -1;
    while (iters--) {
        double time;
        std::tie(time, result) = time_function(std::function<size_t(T)>(Func), max);
        best = std::min(time, best);
    }
    return {best, result};
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
void measure_times(tasks_t tasks, T max, size_t iters) {
    bool test_system = !! (tasks & TEST_SYSTEM);
    bool test_branchfull = !! (tasks & TEST_BRANCHFULL);
    bool test_branchfree = !! (tasks & TEST_BRANCHFREE);
    
    prime_calculation_result_t sys = {0, 0};
    prime_calculation_result_t branchfull = {0, 0};
    prime_calculation_result_t branchfree = {0, 0};
    
    if (test_system) {
        sys = measure_1_prime_calculation<T, count_primes_system<T>>(max, iters);
        std::cout << '.' << std::flush;
    }
    
    if (test_branchfull) {
        branchfull = measure_1_prime_calculation<T, count_primes_libdivide<T, libdivide::BRANCHFULL>>(max, iters);
        std::cout << '.' << std::flush;
    }
    
    if (test_branchfree) {
        branchfree = measure_1_prime_calculation<T, count_primes_libdivide<T, libdivide::BRANCHFREE>>(max, iters);
        std::cout << '.' << std::endl;
    }

    if (test_system && test_branchfull && branchfull.result != sys.result) {
        std::cerr << "Disagreement in branchfull path for type " << typeid(T).name() << ": libdivide says " << branchfull.result << " but system says " << sys.result << std::endl;
        std::exit(1);
    }
    if (test_system && test_branchfree && branchfree.result != sys.result) {
        std::cerr << "Disagreement in branchfree path for type " << typeid(T).name() << ": libdivide says " << branchfree.result << " but system says " << sys.result << std::endl;
        std::exit(1);
    }
    if (test_system) std::cout << "    system: " << sys.duration << " seconds" << std::endl;
    if (test_branchfull) std::cout << "branchfull: " << branchfull.duration << " seconds" << std::endl;
    if (test_branchfree) std::cout << "branchfree: " << branchfree.duration << " seconds" << std::endl;
    std::cout << std::endl;
}

static void usage() {
    std::cout << "Usage: primes [u32] [u64] [s32] [s64] [branchfree] [branchfull] [sys|system]" << std::endl;
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
            std::cout << "Unknown argument '" << arg << "'" << std::endl;
            usage();
            return -1;
        }
    }
    // Set default tasks
    if (! (tasks & TEST_ALL_TYPES)) {
        tasks |= TEST_ALL_TYPES;
    }
    if (! (tasks & TEST_ALL_ALGOS)) {
        tasks |= TEST_ALL_ALGOS;
    }
    
    size_t iters = 3;
    
    if (tasks & TEST_U32) {
        std::cout << "----- u32 -----" << std::endl;
        measure_times<uint32_t>(tasks, 400000, iters);
    }
    
    if (tasks & TEST_S32) {
        std::cout << "----- s32 -----" << std::endl;
        measure_times<int32_t>(tasks, 400000, iters);
    }
    
    if (tasks & TEST_U64) {
        std::cout << "----- u64 -----" << std::endl;
        measure_times<uint64_t>(tasks, 400000, iters);
    }

    if (tasks & TEST_S64) {
        std::cout << "----- s64 -----" << std::endl;
        measure_times<int64_t>(tasks, 400000, iters);
    }

    std::cout << "All tests passed successfully!" << std::endl;

    return 0;
}
