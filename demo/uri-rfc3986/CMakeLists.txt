################################################################################
# Copyright (c) 2021 Vladislav Trifochkin
#
# This file is part of [pfs-parser](https://github.com/semenovf/pfs-parser) library.
################################################################################
project(uri-rfc3986)

add_executable(${PROJECT_NAME} main.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.grammar DESTINATION .)
