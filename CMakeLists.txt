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

# install and export variables
set(config_install_dir "lib/cmake/${PROJECT_NAME}")
set(include_install_dir "include")
set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(project_config "${generated_dir}/${PROJECT_NAME}-config.cmake")
set(version_config "${generated_dir}/${PROJECT_NAME}-config-version.cmake")
set(targets_export_name "${PROJECT_NAME}-targets")
set(namespace "${PROJECT_NAME}::")

# ALIAS same as in configure file
# the ALIAS can be used to create examples which use the same syntax as a client
# application, which uses `find_package(libdivide CONFIG)`
# create the alias libdivide::libdivide
add_library(${namespace}${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# installation of target libdivide
install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${targets_export_name}
    COMPONENT library
    INCLUDES DESTINATION include # set include path for installed library target
)
# installation of header file
install(
    FILES libdivide.h
    DESTINATION include
)


# tester
option(BUILD_TESTER "build tester executable" ON)
if(BUILD_TESTER)
    find_package(Threads)
    add_executable(tester libdivide_test.cpp)
    target_link_libraries(tester ${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
endif(BUILD_TESTER)

# benchmark
option(BUILD_BENCHMARK "build benchmark executable" ON)
if(BUILD_BENCHMARK)
    add_executable(benchmark libdivide_benchmark.c)
    target_link_libraries(benchmark ${PROJECT_NAME})
    target_compile_definitions(benchmark PRIVATE _CRT_SECURE_NO_WARNINGS)
endif(BUILD_BENCHMARK)

# primes
option(BUILD_PRIMES "build primes executable" ON)
if(BUILD_PRIMES)
    add_executable(primes primes_benchmark.cpp)
    target_link_libraries(primes ${PROJECT_NAME})
    set_target_properties(primes PROPERTIES CXX_STANDARD_REQUIRED 11
                                            CXX_STANDARD 14)
endif(BUILD_PRIMES)

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


# Include module for fuctions
# - 'write_basic_package_version_file'
# - 'configure_package_config_file'
include(CMakePackageConfigHelpers)

# enable this block to generate and install a version file
# Configure '<PROJECT-NAME>-config-version.cmake'
# Note: PROJECT_VERSION_STRING needs to be defined
#write_basic_package_version_file(
#    "${version_config}"
#    VERSION ${PROJECT_VERSION_STRING}
#    COMPATIBILITY SameMajorVersion
#)
# install version file
#   * <prefix>/lib/cmake/libdivide/libdivide-config-version.cmake
#install(
#    FILES "${version_config}"
#    DESTINATION "${config_install_dir}"
#)

# Configure '<PROJECT-NAME>-config.cmake'
configure_package_config_file(
    "cmake/config.cmake.in"
    "${project_config}"
    INSTALL_DESTINATION "${config_install_dir}"
)
# install config file
#   * <prefix>/lib/cmake/libdivide/libdivide-config.cmake
install(
    FILES "${project_config}"
    DESTINATION "${config_install_dir}"
)

# install targets file
#   * <prefix>/lib/cmake/libdivide/libdivide-targets.cmake
install(
    EXPORT "${targets_export_name}"
    NAMESPACE "${namespace}"
    DESTINATION "${config_install_dir}"
)
