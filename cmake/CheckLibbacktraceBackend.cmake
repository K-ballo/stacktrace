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
# Resolves libbacktrace via FindLibbacktrace.cmake. Sets <out-var> to a
# boolean, visible to the caller, and EGGS_STACKTRACE_LIBBACKTRACE_HEADER to
# the resolved header path. On success, also defines the INTERFACE library
# _eggs_stacktrace_libbacktrace_support, carrying the library to link as a
# usage requirement (PUBLIC: needed transitively by consumers at final
# link). The header path is deliberately NOT part of this interface
# library: it is only needed to compile the backend's own translation
# unit, so the caller must apply it as a PRIVATE compile definition on
# that target instead - otherwise it would leak the absolute, build-
# machine-specific path to every consumer's compile command.
function(eggs_stacktrace_check_libbacktrace_backend out_var)
    find_package(Libbacktrace QUIET)
    set(${out_var} ${Libbacktrace_FOUND} PARENT_SCOPE)
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
