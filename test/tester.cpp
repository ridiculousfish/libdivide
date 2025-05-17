// Usage: tester [OPTIONS]
//
// You can pass the tester program one or more of the following options:
// u32, s32, u64, s64 or run it without arguments to test all four.
// The tester is multithreaded so it can test multiple cases simultaneously.
// The tester will verify the correctness of libdivide via a set of
// randomly chosen denominators, by comparing the result of libdivide's
// division to hardware division. It may take a long time to run, but it
// will output as soon as it finds a discrepancy.

// Silence MSVC sprintf unsafe warnings
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "DivideTest.h"
#include "libdivide.h"

// This is simply a regression test for #96: that the following all compile (and don't crash).
static void test_primitives_compile() {
    libdivide::divider<short> d0(1);
    libdivide::divider<int> d1(1);
    libdivide::divider<long> d2(1);
    libdivide::divider<long long> d3(1);

    libdivide::divider<unsigned short> u0(1);
    libdivide::divider<unsigned int> u1(1);
    libdivide::divider<unsigned long> u2(1);
    libdivide::divider<unsigned long long> u3(1);

    // These should fail to compile.
    // libdivide::divider<float> f0(1);
    // libdivide::divider<double> f1(1);
}

enum TestType {
    type_s16,
    type_u16,
    type_s32,
    type_u32,
    type_s64,
    type_u64,
};

void wait_for_threads(std::vector<std::thread> &test_threads) {
    for (auto &t : test_threads) {
        t.join();
    }
}

uint8_t get_max_threads() { return (uint8_t)std::max(1U, std::thread::hardware_concurrency()); }

template <typename _IntT>
void launch_test_thread(std::vector<std::thread> &test_threads) {
    static uint8_t max_threads = get_max_threads();

    if (max_threads == test_threads.size()) {
        wait_for_threads(test_threads);
        test_threads.clear();
    }
    test_threads.emplace_back(run_test<_IntT>);
}

int main(int argc, char *argv[]) {
    bool default_do_test = (argc <= 1);
    std::vector<bool> do_tests(6, default_do_test);

    test_primitives_compile();

    for (int i = 1; i < argc; i++) {
        const std::string arg(argv[i]);
        if (arg == type_tag<int16_t>::get_tag())
            do_tests[type_s16] = true;
        else if (arg == type_tag<uint16_t>::get_tag())
            do_tests[type_u16] = true;
        else if (arg == type_tag<int32_t>::get_tag())
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
                   "u16, s16, u32, s32, u64, s64 or run it without arguments to test all four.\n"
                   "The tester is multithreaded so it can test multiple cases simultaneously.\n"
                   "The tester will verify the correctness of libdivide via a set of\n"
                   "randomly chosen denominators, by comparing the result of libdivide's\n"
                   "division to hardware division. It may take a long time to run, but it\n"
                   "will output as soon as it finds a discrepancy."
                << std::endl;
            exit(1);
        }
    }

    std::cout << "Testing libdivide v" << LIBDIVIDE_VERSION << std::endl;
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

    if (do_tests[type_s16]) {
        launch_test_thread<int16_t>(test_threads);
    }
    if (do_tests[type_u16]) {
        launch_test_thread<uint16_t>(test_threads);
    }
    if (do_tests[type_s32]) {
        launch_test_thread<int32_t>(test_threads);
    }
    if (do_tests[type_u32]) {
        launch_test_thread<uint32_t>(test_threads);
    }
    if (do_tests[type_s64]) {
        launch_test_thread<int64_t>(test_threads);
    }
    if (do_tests[type_u64]) {
        launch_test_thread<uint64_t>(test_threads);
    }

    wait_for_threads(test_threads);

    std::cout << "\nAll tests passed successfully!" << std::endl;
    return 0;
}
