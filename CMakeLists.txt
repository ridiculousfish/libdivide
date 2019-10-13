cmake_minimum_required(VERSION 3.4)
set(LIBDIVIDE_VERSION "3.0")
project(libdivide C CXX)

include(CheckCXXCompilerFlag)
include(CheckCXXSourceRuns)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(CMakePushCheckState)

# Build options ################################################

option(BUILD_TESTS "Build the test programs" ON)
option(ENABLE_VECTOR_EXTENSIONS "Enable CPU vector instructions for test programs" ON)

# By default we automatically enable the widest vector
# instruction set supported by your x86/x64 CPU.
# But you can also force a specific vector instruction
# set using the options below.

option(LIBDIVIDE_SSE2   "Enable SSE2 vector instructions"   OFF)
option(LIBDIVIDE_AVX2   "Enable AVX2 vector instructions"   OFF)
option(LIBDIVIDE_AVX512 "Enable AVX512 vector instructions" OFF)

# By default enable release mode ###############################

if(NOT CMAKE_VERSION VERSION_LESS 3.9)
    get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
elseif(CMAKE_CONFIGURATION_TYPES)
    set(isMultiConfig TRUE)
endif()

if(NOT isMultiConfig AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
    "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

# Enable assertions in debug mode ##############################

string(TOUPPER "${CMAKE_BUILD_TYPE}" BUILD_TYPE)

if("${BUILD_TYPE}" MATCHES DEBUG)
    set(LIBDIVIDE_ASSERTIONS -DLIBDIVIDE_ASSERTIONS_ON)
endif()

# Detect cross compilation #####################################

if(CMAKE_SYSTEM_NAME AND
   CMAKE_HOST_SYSTEM_NAME AND
   NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "${CMAKE_HOST_SYSTEM_NAME}")
    set(IS_CROSS_COMPILIATION ON)
endif()

if(CMAKE_SYSTEM_PROCESSOR AND
   CMAKE_HOST_SYSTEM_PROCESSOR AND
   NOT "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "${CMAKE_HOST_SYSTEM_PROCESSOR}")
    set(IS_CROSS_COMPILIATION ON)
endif()

# Check if x86/x64 CPU  ########################################

if (BUILD_TESTS AND ENABLE_VECTOR_EXTENSIONS AND NOT IS_CROSS_COMPILIATION)
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

    if (cpu_x86)
        set(ENABLE_X86_VECTOR_EXTENSIONS ON)

        cmake_push_check_state()
        set(CMAKE_REQUIRED_FLAGS -Werror)
        check_cxx_compiler_flag(-march=native march_native)
        cmake_pop_check_state()

        if (march_native)
            # -march=native required for AVX2/AVX512 on x86
            set(NATIVE_FLAG -march=native)
        endif()
    endif()
endif()

# Disable auto vectorization ###################################

# We disable auto vectorization on x86 (and x64-64) in order
# to prevent the compiler from vectorizing our scalar benchmarks
# which would make the benchmark results less useful.

if(ENABLE_X86_VECTOR_EXTENSIONS)
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        check_cxx_compiler_flag("-fno-vectorize" fno_vectorize)
        if(fno_vectorize)
            set(NO_VECTORIZE -fno-vectorize)
        endif()
    elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        check_cxx_compiler_flag("-fno-tree-vectorize" fno_tree_vectorize)
        if(fno_tree_vectorize)
            set(NO_VECTORIZE -fno-tree-vectorize)
        endif()
    endif()
endif()

# Check if CPU supports AVX512/AVX2/SSE2 #######################

if(LIBDIVIDE_AVX512)
    set(LIBDIVIDE_VECTOR_EXT -DLIBDIVIDE_AVX512)
elseif(LIBDIVIDE_AVX2)
    set(LIBDIVIDE_VECTOR_EXT -DLIBDIVIDE_AVX2)
elseif(LIBDIVIDE_SSE2)
    set(LIBDIVIDE_VECTOR_EXT -DLIBDIVIDE_SSE2)
elseif(ENABLE_X86_VECTOR_EXTENSIONS)

    cmake_push_check_state()
    if(march_native)
        set(CMAKE_REQUIRED_FLAGS "-march=native")
    endif()

    check_cxx_source_runs("
        #include <immintrin.h>
        int main()
        {
            __m512i a = _mm512_set_epi32(1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0);
            __m512i b = _mm512_set_epi32(0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1);
            __m512i c = _mm512_add_epi32(a, b);
            __m512i d = _mm512_srai_epi32(c, 23);
            d = _mm512_shuffle_epi32(d, (_MM_PERM_ENUM) 0xB1);
            return 0;
        }"
        avx512)

    if(avx512)
        set(LIBDIVIDE_VECTOR_EXT -DLIBDIVIDE_AVX512)
    else()
        check_cxx_source_runs("
            #include <immintrin.h>
            int main()
            {
                __m256i a = _mm256_set_epi32(1, 0, 1, 0, 1, 0, 1, 0);
                __m256i b = _mm256_set_epi32(0, 1, 0, 1, 0, 1, 0, 1);
                b = _mm256_add_epi32(a, b);
                return 0;
            }"
            avx2)

        if(avx2)
            set(LIBDIVIDE_VECTOR_EXT -DLIBDIVIDE_AVX2)
        else()
            check_cxx_source_runs("
                #include <emmintrin.h>
                int main()
                {
                    __m128i a = _mm_set_epi32(1, 0, 1, 0);
                    __m128i b = _mm_set_epi32(0, 1, 0, 1);
                    b = _mm_add_epi32(a, b);
                    return 0;
                }"
                sse2)

            if(sse2)
                set(LIBDIVIDE_VECTOR_EXT -DLIBDIVIDE_SSE2)
            endif()
        endif()
    endif()

    cmake_pop_check_state()
endif()

# libdivide header-only library target #########################

add_library(libdivide INTERFACE)
add_library(libdivide::libdivide ALIAS libdivide)

target_compile_features(libdivide INTERFACE cxx_alias_templates)

target_include_directories(libdivide INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>)

install(FILES libdivide.h DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

# CMake find_package(libdivide) support ########################

install(TARGETS libdivide
        EXPORT libdivideConfig)

export(TARGETS libdivide
       NAMESPACE libdivide::
       FILE "${CMAKE_CURRENT_BINARY_DIR}/libdivideConfig.cmake")

install(EXPORT libdivideConfig
        NAMESPACE libdivide::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/libdivide")

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/libdivideConfigVersion.cmake"
    VERSION ${LIBDIVIDE_VERSION}
    COMPATIBILITY SameMajorVersion)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libdivideConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/libdivide")

# Build test programs ##########################################

if (BUILD_TESTS)
    find_package(Threads REQUIRED QUIET)

    add_executable(tester test/tester.cpp)
    add_executable(benchmark test/benchmark.c)
    add_executable(benchmark_branchfree test/benchmark_branchfree.cpp)

    target_link_libraries(tester libdivide Threads::Threads)
    target_link_libraries(benchmark libdivide)
    target_link_libraries(benchmark_branchfree libdivide)

    target_compile_options(tester PRIVATE "${NATIVE_FLAG}" "${NO_VECTORIZE}")
    target_compile_options(benchmark PRIVATE "${NATIVE_FLAG}" "${NO_VECTORIZE}")
    target_compile_options(benchmark_branchfree PRIVATE "${NATIVE_FLAG}" "${NO_VECTORIZE}")

    target_compile_definitions(tester PRIVATE "${LIBDIVIDE_ASSERTIONS}" "${LIBDIVIDE_VECTOR_EXT}")
    target_compile_definitions(benchmark PRIVATE "${LIBDIVIDE_ASSERTIONS}" "${LIBDIVIDE_VECTOR_EXT}")
    target_compile_definitions(benchmark_branchfree PRIVATE "${LIBDIVIDE_ASSERTIONS}" "${LIBDIVIDE_VECTOR_EXT}")
endif()

# Enable testing ###############################################

if (BUILD_TESTS)
    enable_testing()
    add_test(tester tester)
    add_test(benchmark_branchfree benchmark_branchfree)
endif()
