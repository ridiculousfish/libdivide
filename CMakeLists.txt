cmake_minimum_required(VERSION 3.1)
project(libdivide)

include(CheckCXXCompilerFlag)
include(CMakePushCheckState)

cmake_push_check_state()
check_cxx_compiler_flag(-msse2 MSSE2_FLAG_AVAILABLE)
cmake_pop_check_state()

option(USE_SSE2 "use SSE2 instructions" ${MSSE2_FLAG_AVAILABLE})

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_compile_definitions(${PROJECT_NAME} INTERFACE $<$<CONFIG:Debug>:LIBDIVIDE_ASSERTIONS_ON=1>)
target_compile_options(${PROJECT_NAME} INTERFACE
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-fstrict-aliasing -Wall -Wextra>
    )
if(USE_SSE2)
    message(STATUS "SSE2 is used")
    target_compile_options(${PROJECT_NAME} INTERFACE
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-msse2>
        )
    target_compile_definitions(${PROJECT_NAME} INTERFACE
        LIBDIVIDE_USE_SSE2=1
        )
else(USE_SSE2)
    message(STATUS "SSE2 is not used")
endif(USE_SSE2)


# tester
option(BUILD_TESTER "build tester executable" ON)
if(BUILD_TESTER)
    find_package(Threads)
    add_executable(tester libdivide_test.cpp)
    target_link_libraries(tester ${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
endif(BUILD_TESTER)

# CTest
option(ENABLE_TESTING "enable testing with CTest" ON)
if(ENABLE_TESTING)
    enable_testing()

    if(BUILD_TESTER)
        add_test(tester-u32 tester "u32")
        add_test(tester-u64 tester "u64")
        add_test(tester-s32 tester "s32")
        add_test(tester-s64 tester "s64")
    endif(BUILD_TESTER)
endif(ENABLE_TESTING)
