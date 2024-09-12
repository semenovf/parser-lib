################################################################################
# Copyright (c) 2021-2024 Vladislav Trifochkin
#
# This file is part of `parser-lib`.
################################################################################
cmake_minimum_required (VERSION 3.11)
project(parser-ALL CXX C)

option(PARSER__BUILD_STRICT "Build with strict policies: C++ standard required, C++ extension is OFF etc" ON)
option(PARSER__BUILD_TESTS "Build tests" OFF)
option(PARSER__BUILD_DEMO "Build examples/demo" OFF)
option(PARSER__ENABLE_COVERAGE "Build tests with coverage support" OFF)

if (NOT PORTABLE_TARGET__ENABLED)
    list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/2ndparty/portable-target/cmake/v2)
    include(PortableTarget)
    portable_target(SET ENABLE_TRACE ON)
endif()

message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ compiler version: ${CMAKE_CXX_COMPILER_VERSION}")

if (PARSER__BUILD_STRICT)
    set(CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF) # use -std=c++XX rather than -std=gnu++XX
endif()

include(${CMAKE_CURRENT_LIST_DIR}/library.cmake)

if (PARSER__BUILD_TESTS AND EXISTS ${CMAKE_CURRENT_LIST_DIR}/tests)
    enable_testing()
    add_subdirectory(tests)
endif()

if (PARSER__BUILD_DEMO AND EXISTS ${CMAKE_CURRENT_LIST_DIR}/demo)
    add_subdirectory(demo)
endif()
