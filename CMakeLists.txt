################################################################################
# Copyright (c) 2021 Vladislav Trifochkin
#
# This file is part of [pfs-parser](https://github.com/semenovf/pfs-parser) library.
################################################################################
cmake_minimum_required (VERSION 3.5)
project(pfs-parser VERSION 1.0 LANGUAGES CXX)

string(REPLACE "-" "::" _project_alias ${PROJECT_NAME})
set(_project_basename ${PROJECT_NAME})

option(${PROJECT_NAME}_BUILD_TESTS "Build tests" OFF)
option(${PROJECT_NAME}_BUILD_EXAMPLES "Build examples" OFF)

message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ compiler version: ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Project alias: ${_project_alias}")

set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # use -std=c++XX rather than -std=gnu++XX

include(${CMAKE_SOURCE_DIR}/cmake/cxx20_concepts.cmake)

add_library(${PROJECT_NAME} INTERFACE)
add_library(${_project_alias} ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

if (PFS_CXX_HAVE_CONCEPTS)
    target_compile_definitions(${PROJECT_NAME} INTERFACE PFS_CXX_HAVE_CONCEPTS)
endif()

if (${PROJECT_NAME}_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if (${PROJECT_NAME}_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
