cmake_minimum_required(VERSION 3.1)
project(libdivide)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_compile_definitions(${PROJECT_NAME} INTERFACE $<$<CONFIG:Debug>:LIBDIVIDE_ASSERTIONS_ON=1>)
target_compile_options(${PROJECT_NAME} INTERFACE
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-fstrict-aliasing -Wall -Wextra>
    )
