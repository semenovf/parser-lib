project(pfs-parser_TESTS)
option(pfs-parser_COVERAGE "Build tests with coverage support" OFF)

if (NOT TARGET pfs::parser)
    message(FATAL_ERROR "No target `pfs::json` found")
endif()

if (pfs-parser_COVERAGE)
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
    core_rules
    abnf_rules)

foreach (name ${TEST_NAMES})
    if (${name}_SOURCES)
        add_executable(${name} ${name}.cpp ${${name}_SOURCES})
    else ()
        add_executable(${name} ${name}.cpp)
    endif()

    target_link_libraries(${name} PRIVATE pfs::parser)
    add_test(NAME ${name} COMMAND ${name})
endforeach()
