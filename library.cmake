################################################################################
# Copyright (c) 2024 Vladislav Trifochkin
#
# This file is part of `parser-lib`
################################################################################
cmake_minimum_required (VERSION 3.5)
project(parser CXX C)

portable_target(ADD_INTERFACE ${PROJECT_NAME} ALIAS pfs::parser)
portable_target(INCLUDE_DIRS ${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
