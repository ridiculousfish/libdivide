cmake_minimum_required(VERSION 3.1)
project(libdivide)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_compile_definitions(${PROJECT_NAME} INTERFACE $<$<CONFIG:Debug>:LIBDIVIDE_ASSERTIONS_ON=1>)
target_compile_options(${PROJECT_NAME} INTERFACE
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-fstrict-aliasing -Wall -Wextra>
    )


# tester
option(BUILD_TESTER "build tester executable" ON)
if(BUILD_TESTER)
    find_package(Threads)
    add_executable(tester libdivide_test.cpp)
    target_link_libraries(tester ${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
endif(BUILD_TESTER)
