cmake_minimum_required(VERSION 3.1)
project(libdivide C CXX)
include(CheckCXXCompilerFlag)
include(CheckCXXSourceRuns)
include(CMakePushCheckState)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DLIBDIVIDE_ASSERTIONS_ON")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DLIBDIVIDE_ASSERTIONS_ON")

# Build options ################################################

# By default we automatically enable the widest vector
# instruction set supported by your x86/x64 CPU.
# But you can also force a specific vector instruction
# set using the options below.

option(LIBDIVIDE_USE_SSE2   "Enable SSE2 vector instructions"   OFF)
option(LIBDIVIDE_USE_AVX2   "Enable AVX2 vector instructions"   OFF)
option(LIBDIVIDE_USE_AVX512 "Enable AVX512 vector instructions" OFF)

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

# -march=native required for AVX2/AVX512 ######################

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

# Disable auto vectorization ##################################

# We disable auto vectorization in order to prevent the
# compiler from vectorizing our scalar benchmarks
# which would make the benchmark results less useful.

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    check_cxx_compiler_flag("-fno-vectorize" fno_vectorize)
    if(fno_vectorize)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-vectorize")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-vectorize")
    endif()
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    check_cxx_compiler_flag("-fno-tree-vectorize" fno_tree_vectorize)
    if(fno_tree_vectorize)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-tree-vectorize")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-tree-vectorize")
    endif()
endif()

# Check if CPU supports AVX512/AVX2/SSE2 #######################

if(LIBDIVIDE_USE_AVX512)
    add_definitions(-DLIBDIVIDE_USE_AVX512)
elseif(LIBDIVIDE_USE_AVX2)
    add_definitions(-DLIBDIVIDE_USE_AVX2)
elseif(LIBDIVIDE_USE_SSE2)
    add_definitions(-DLIBDIVIDE_USE_SSE2)
elseif(cpu_x86)
    check_cxx_source_runs("
        #include <immintrin.h>
        int main()
        {
            __m512i a = _mm512_set_epi32(1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0);
            __m512i b = _mm512_set_epi32(0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1);
            __m512i c = _mm512_add_epi32(a, b);
            __m512i d = _mm512_srai_epi32(c, 23);
            __m512i e = _mm512_shuffle_epi32(d, (_MM_PERM_ENUM) 0xB1);
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

# Enable testing ###############################################

enable_testing()
add_test(tester tester)
add_test(primes_benchmark primes_benchmark)

# By default install to /usr/local/install #####################

install(FILES libdivide.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include)
