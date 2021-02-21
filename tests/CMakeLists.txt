################################################################################
# Copyright (c) 2021 Vladislav Trifochkin
#
# This file is part of [pfs-parser](https://github.com/semenovf/pfs-parser) library.
################################################################################
project(${_project_basename}_TESTS)

option(${_project_basename}_ENABLE_COVERAGE "Build tests with coverage support" OFF)

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
        add_executable(${name} ${name}.cpp ${${name}_SOURCES})
    else ()
        add_executable(${name} ${name}.cpp)
    endif()

    # https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html#Instrumentation-Options
    #
    # --coverage
    #       This option is used to compile and link code instrumented for
    #       coverage analysis. The option is a synonym for -fprofile-arcs
    #       -ftest-coverage (when compiling) and -lgcov (when linking).
    if (${_project_basename}_ENABLE_COVERAGE)
        target_compile_options(${name} PRIVATE -g -O0 --coverage)
        target_link_libraries(${name} PRIVATE -g -O0 --coverage)
    endif()

    target_link_libraries(${name} PRIVATE pfs::parser)
    add_test(NAME ${name} COMMAND ${name})
endforeach()

if (${_project_basename}_ENABLE_COVERAGE)
    include(${CMAKE_SOURCE_DIR}/cmake/Coverage.cmake)
    coverage_target("'/usr/*';'*/doctest.h'")
endif()
