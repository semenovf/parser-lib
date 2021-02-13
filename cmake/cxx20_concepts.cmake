################################################################################
# Copyright (c) 2021 Vladislav Trifochkin
#
# This file is part of [pfs-common](https://github.com/semenovf/pfs-common) library.
#
# Checks compiler supports Concepts
#
# Changelog:
#       2021.02.08 Initial version
################################################################################
include(CMakePushCheckState)
include(CheckIncludeFileCXX)
include(CheckCXXSourceCompiles)

# This is OBSOLETE.
# Use in-source check:
# #if __cplusplus > 201703L && __cpp_concepts >= 201907L
#       ...
# #endif

# Examples from:
# https://devblogs.microsoft.com/cppblog/c20-concepts-are-here-in-visual-studio-2019-version-16-3/

cmake_push_check_state()

set(__have_concepts FALSE)
check_include_file_cxx("concepts" _have_std_concepts_header)

# TODO need to compile with specific options (`-std=c++2a`, `-std=c++20` or `-fconcepts` for g++)
# These options must be set before for project
if (_have_std_concepts_header)
    string(CONFIGURE [[
        #include <concepts>

        // This concept tests whether 'T::type' is a valid type
        template<typename T>
        concept has_type_member = requires { typename T::type; };
    ]] _check_cxx_code @ONLY)

    # Try to compile a simple program without any linker flags
    check_cxx_source_compiles("${_check_cxx_code}" _concepts_compiled)

    if (_concepts_compiled)
        set(_have_concepts TRUE)
    endif()
endif()

if (${_have_concepts})
    set(PFS_CXX_HAVE_CONCEPTS ON CACHE BOOL "C++ compiler supports Concepts")
    set(_have_concepts_yesno "Yes")
else()
    set(_have_concepts_yesno "No")
endif()

message(STATUS "C++ compiler supports Concepts: ${_have_concepts_yesno}")

cmake_pop_check_state()
