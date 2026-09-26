# Eggs.Stacktrace
#
# Copyright (c) 2026 Agustin Berge
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

include_guard()

include(CheckCXXSourceCompiles)

# eggs_stacktrace_check_std_stacktrace(<out-var>)
#
# Checks whether std::stacktrace links in C++23 mode, and which additional
# library it needs (libstdc++ ships it separately as stdc++exp, or as
# stdc++_libbacktrace before GCC 13.3). Sets <out-var> to that library name,
# or to an empty string if std::stacktrace needs no additional library or is
# not available at all.
function(eggs_stacktrace_check_std_stacktrace out_var)
    set(${out_var} "" PARENT_SCOPE)
    if(NOT "cxx_std_23" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
        return()
    endif()

    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_REQUIRED_QUIET ON)
    set(_source
        "
        #include <stacktrace>
        int main()
        {
            return static_cast<int>(std::stacktrace::current().size());
        }
        "
    )

    # "none" stands for linking no additional library.
    foreach(_library IN ITEMS none stdc++exp stdc++_libbacktrace)
        string(MAKE_C_IDENTIFIER "${_library}" _id)
        set(CMAKE_REQUIRED_LIBRARIES)
        if(NOT _library STREQUAL "none")
            set(CMAKE_REQUIRED_LIBRARIES ${_library})
        endif()
        check_cxx_source_compiles(
            "${_source}"
            EGGS_STACKTRACE_STD_STACKTRACE_LINKS_${_id}
        )
        if(EGGS_STACKTRACE_STD_STACKTRACE_LINKS_${_id})
            set(${out_var} "${CMAKE_REQUIRED_LIBRARIES}" PARENT_SCOPE)
            return()
        endif()
    endforeach()
endfunction()
