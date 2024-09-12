################################################################################
# Copyright (c) 2021-2024 Vladislav Trifochkin
#
# This file is part of `parser-lib`.
################################################################################
project(parser_TESTS CXX C)

if (PFS__ENABLE_COVERAGE)
    message(STATUS "Coverage feature enabled")
    if (NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        message(FATAL_ERROR "Coverage feature requires GCC")
    endif()

    set(CMAKE_CXX_FLAGS "-g -O0 --coverage")
    set(CMAKE_C_FLAGS "-g -O0 --coverage")
endif()

# Copy test files to build folder
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data DESTINATION .)

set(TEST_NAMES
    generator
    core_rules
    abnf_rules
    parse_files
    syntax_tree)

foreach (name ${TEST_NAMES})
    if (${name}_SOURCES)
        add_executable(${name} ${${name}_SOURCES} ${name}.cpp)
    else ()
        add_executable(${name} ${name}.cpp)
    endif()

    # https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html#Instrumentation-Options
    #
    # --coverage
    #       This option is used to compile and link code instrumented for
    #       coverage analysis. The option is a synonym for -fprofile-arcs
    #       -ftest-coverage (when compiling) and -lgcov (when linking).
    if (PFS__ENABLE_COVERAGE)
        target_compile_options(${name} PRIVATE -g -O0 --coverage)
        target_link_libraries(${name} PRIVATE -g -O0 --coverage)
    endif()

    target_link_libraries(${name} PRIVATE pfs::parser)
    add_test(NAME ${name} COMMAND ${name})
endforeach()

if (PFS__ENABLE_COVERAGE)
    include(${CMAKE_SOURCE_DIR}/cmake/Coverage.cmake)
    coverage_target("'/usr/*';'*/doctest.h'")
endif()
