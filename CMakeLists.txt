cmake_minimum_required(VERSION 3.1)
project(libdivide C CXX)
include(CheckCXXCompilerFlag)
include(CheckCXXSourceRuns)
include(CMakePushCheckState)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DLIBDIVIDE_ASSERTIONS_ON")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DLIBDIVIDE_ASSERTIONS_ON")

# By default enable release mode ###############################

string(TOUPPER "${CMAKE_BUILD_TYPE}" BUILD_TYPE)

if(NOT "${BUILD_TYPE}" MATCHES DEBUG)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Check if x86/x64 CPU  ########################################

check_cxx_source_runs("
    int main()
    {
        #if !defined(__i386__) && \
            !defined(__x86_64__) && \
            !defined(_M_IX86) && \
            !defined(_M_X64)
            Compile error: not x86 CPU architecture
        #endif
        return 0;
    }"
    cpu_x86)

# Check -march=native compiler flag ############################

if (cpu_x86)
    cmake_push_check_state()
    set(CMAKE_REQUIRED_FLAGS -Werror)
    check_cxx_compiler_flag(-march=native march_native)
    cmake_pop_check_state()

    if(march_native)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
    endif()
endif()

# Check if CPU supports AVX512/AVX2/SSE2 #######################

if(cpu_x86 AND march_native)
    check_cxx_source_runs("
        #include <immintrin.h>
        int main()
        {
            __m512i a = _mm512_set_epi32(1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0);
            __m512i b = _mm512_set_epi32(0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1);
            __m512i c = _mm512_add_epi32(a, b);
            __m512i d = _mm512_srai_epi32(c, 23);
            return 0;
        }"
        avx512)

    if(avx512)
        add_definitions(-DLIBDIVIDE_USE_AVX512)
    else()
        check_cxx_source_runs("
            #include <immintrin.h>
            int main()
            {
                __m256i a = _mm256_set_epi32(1, 0, 1, 0, 1, 0, 1, 0);
                __m256i b = _mm256_set_epi32(0, 1, 0, 1, 0, 1, 0, 1);
                __m256i c = _mm256_add_epi32(a, b);
                return 0;
            }"
            avx2)

        if(avx2)
            add_definitions(-DLIBDIVIDE_USE_AVX2)
        else()
            check_cxx_source_runs("
                #include <emmintrin.h>
                int main()
                {
                    __m128i a = _mm_set_epi32(1, 0, 1, 0);
                    __m128i b = _mm_set_epi32(0, 1, 0, 1);
                    __m128i c = _mm_add_epi32(a, b);
                    return 0;
                }"
                sse2)

            if(sse2)
                add_definitions(-DLIBDIVIDE_USE_SSE2)
            endif()
        endif()
    endif()
endif()

# Build test programs ##########################################

find_package(Threads REQUIRED QUIET)

add_executable(tester test/tester.cpp)
add_executable(benchmark test/benchmark.c)
add_executable(primes_benchmark test/primes_benchmark.cpp)

target_include_directories(tester PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(benchmark PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(primes_benchmark PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(tester Threads::Threads)

# Disable auto vectorization for benchmark #####################

if(avx512 OR avx2 OR sse2)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        check_cxx_compiler_flag("-fno-slp-vectorize" fno_slp_vectorize)
        if(fno_slp_vectorize)
            target_compile_options(benchmark PRIVATE "-fno-slp-vectorize")
        endif()
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        check_cxx_compiler_flag("-fno-tree-vectorize" fno_tree_vectorize)
        if(fno_tree_vectorize)
            target_compile_options(benchmark PRIVATE "-fno-tree-vectorize")
        endif()
    endif()
endif()

# Enable testing ###############################################

enable_testing()
add_test(tester tester)
add_test(primes_benchmark primes_benchmark)

# By default install to /usr/local/install #####################

install(FILES libdivide.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
