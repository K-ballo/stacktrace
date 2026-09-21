# Eggs.Stacktrace
#
# Copyright (c) 2026 Agustin Berge
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

include_guard()

# So find_package(Libbacktrace) below locates our FindLibbacktrace.cmake
# regardless of the caller's own CMAKE_MODULE_PATH.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")

# eggs_stacktrace_check_libbacktrace_backend(<out-var>)
#
# Checks whether libbacktrace is present and links successfully.
# Sets <out-var> as an internal cache boolean. On success, also defines the
# INTERFACE library _eggs_stacktrace_libbacktrace_support. Additionaly,
# sets EGGS_STACKTRACE_LIBBACKTRACE_HEADER to the resolved header path..
function(eggs_stacktrace_check_libbacktrace_backend out_var)
    find_package(Libbacktrace QUIET)
    set(${out_var} ${Libbacktrace_FOUND} CACHE INTERNAL "")
    if(NOT Libbacktrace_FOUND)
        return()
    endif()

    set(EGGS_STACKTRACE_LIBBACKTRACE_HEADER
        "${Libbacktrace_HEADER}"
        PARENT_SCOPE
    )
    if(NOT TARGET _eggs_stacktrace_libbacktrace_support)
        add_library(_eggs_stacktrace_libbacktrace_support INTERFACE)
        target_link_libraries(
            _eggs_stacktrace_libbacktrace_support
            INTERFACE Libbacktrace::Libbacktrace
        )
    endif()
endfunction()
