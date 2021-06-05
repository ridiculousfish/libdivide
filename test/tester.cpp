// Usage: tester [OPTIONS]
//
// You can pass the tester program one or more of the following options:
// u32, s32, u64, s64 or run it without arguments to test all four.
// The tester is multithreaded so it can test multiple cases simultaneously.
// The tester will verify the correctness of libdivide via a set of
// randomly chosen denominators, by comparing the result of libdivide's
// division to hardware division. It may take a long time to run, but it
// will output as soon as it finds a discrepancy.

#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include "DivideTest.h"
#include "libdivide.h"

enum TestType {
    type_s32,
    type_u32,
    type_s64,
    type_u64,
};

int main(int argc, char *argv[]) {
    bool default_do_test = (argc <= 1);
    std::vector<bool> do_tests(4, default_do_test);

    for (int i = 1; i < argc; i++) {
        const std::string arg(argv[i]);
        if (arg == type_tag<int32_t>::get_tag())
            do_tests[type_s32] = true;
        else if (arg == type_tag<uint32_t>::get_tag())
            do_tests[type_u32] = true;
        else if (arg == type_tag<int64_t>::get_tag())
            do_tests[type_s64] = true;
        else if (arg == type_tag<uint64_t>::get_tag())
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
#if defined(LIBDIVIDE_NEON)
    vecTypes += "neon ";
#endif
    if (vecTypes.empty()) {
        vecTypes = "none ";
    }
    vecTypes.back() = '\n';  // trailing space
    std::cout << "Testing with SIMD ISAs: " << vecTypes;

    // Run tests in threads.
    std::vector<std::thread> test_threads;
    if (do_tests[type_s32]) {
        test_threads.emplace_back(run_test<int32_t>);
    }
    if (do_tests[type_u32]) {
        test_threads.emplace_back(run_test<uint32_t>);
    }
    if (do_tests[type_s64]) {
        test_threads.emplace_back(run_test<int64_t>);
    }
    if (do_tests[type_u64]) {
        test_threads.emplace_back(run_test<uint64_t>);
    }
    for (auto &t : test_threads) {
        t.join();
    }

    std::cout << "\nAll tests passed successfully!" << std::endl;
    return 0;
}
