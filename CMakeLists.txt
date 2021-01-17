cmake_minimum_required (VERSION 3.5)
project(pfs-parser CXX)

option(${PROJECT_NAME}_BUILD_TESTS "Build tests" OFF)

################################################################################
# Find `pfs::common` library root directory and configure it
################################################################################
if (NOT TARGET pfs::common)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/FindDirectory.cmake)
    find_directory(pfs-common_DIR
        FILENAME pfs_common_library.hpp
        RELATIVE ${CMAKE_CURRENT_LIST_DIR})

    if (NOT pfs-common_DIR)
        message(FATAL_ERROR "pfs::common library directory not found")
    endif()

    message(STATUS "pfs::common directory found at: [${pfs-common_DIR}]")

    add_subdirectory(${pfs-common_DIR})
endif()
################################################################################

add_library(${PROJECT_NAME} INTERFACE)
add_library(pfs::parser ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(${PROJECT_NAME} INTERFACE pfs::common)

if (${PROJECT_NAME}_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
