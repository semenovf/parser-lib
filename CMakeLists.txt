cmake_minimum_required (VERSION 3.5)
project(pfs-parser CXX)

option(${PROJECT_NAME}_BUILD_TESTS "Build tests" OFF)

add_library(${PROJECT_NAME} INTERFACE)
add_library(pfs::parser ALIAS ${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

if (${PROJECT_NAME}_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
