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

# Enable GCC/Clang warnings ####################################

set(WALL_FLAGS "-Wall -Wno-unknown-pragmas -fstrict-aliasing")
check_cxx_compiler_flag("${WALL_FLAGS}" wall)

if(wall)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WALL_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WALL_FLAGS}")
endif()

# Check if compiler supports AVX2 ##############################

cmake_push_check_state()
set(CMAKE_REQUIRED_FLAGS -Werror)
check_cxx_compiler_flag(-mavx2 mavx2)
cmake_pop_check_state()

# Check if CPU supports AVX2 ###################################

if(mavx2)
    check_cxx_source_runs("
        #include <immintrin.h>
        int main()
        {
            __m256i a = _mm256_set_epi32(1, 0, 1, 0, 1, 0, 1, 0);
            __m256i b = _mm256_set_epi32(0, 1, 0, 1, 0, 1, 0, 1);
            __m256i c = _mm256_add_epi32(a, b);
            __m256i d = _mm256_sra_epi32(c, 23);
            return 0;
        }"
        cpu_avx2)

    if(cpu_avx2)
        add_definitions(-DLIBDIVIDE_USE_AVX2)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mavx2")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2")
    endif()
endif()

# Check -msse2 compiler flag ###################################

if (NOT cpu_avx2)
    cmake_push_check_state()
    set(CMAKE_REQUIRED_FLAGS -Werror)
    check_cxx_compiler_flag(-msse2 msse2)
    cmake_pop_check_state()

    if(msse2)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse2")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2")
        add_definitions(-DLIBDIVIDE_USE_SSE2)
    endif()
endif()

# Build test programs ##########################################

find_package(Threads REQUIRED)

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
