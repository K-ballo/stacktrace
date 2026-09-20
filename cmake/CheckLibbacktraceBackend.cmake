# Eggs.Stacktrace
#
# Copyright (c) 2026 Agustin Berge
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

include_guard()

# eggs_stacktrace_check_libbacktrace_backend(<out-var>)
#
# Locates libbacktrace (Ian Lance Taylor, distributed with GCC), which ships
# either as a standalone install or bundled inside a GCC compiler
# installation. Tries the standalone install first, then asks the compiler
# where its private copy lives; -print-file-name echoes the bare name back
# unchanged when absent. Sets <out-var> to a boolean, visible to the
# caller, and EGGS_STACKTRACE_LIBBACKTRACE_HEADER to the resolved header
# path. On success, also defines the INTERFACE library
# _eggs_stacktrace_libbacktrace_support, carrying the library to link as a
# usage requirement (PUBLIC: needed transitively by consumers at final
# link). The header path is deliberately NOT part of this interface
# library: it is only needed to compile the backend's own translation
# unit, so the caller must apply it as a PRIVATE compile definition on
# that target instead - otherwise it would leak the absolute, build-
# machine-specific path to every consumer's compile command.
function(eggs_stacktrace_check_libbacktrace_backend out_var)
    find_library(EGGS_STACKTRACE_LIBBACKTRACE_LIBRARY NAMES backtrace)
    if(
        NOT EGGS_STACKTRACE_LIBBACKTRACE_LIBRARY
        AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang"
    )
        execute_process(
            COMMAND "${CMAKE_CXX_COMPILER}" -print-file-name=libbacktrace.a
            OUTPUT_VARIABLE _eggs_stacktrace_libbacktrace_path
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        if(
            _eggs_stacktrace_libbacktrace_path
            AND NOT _eggs_stacktrace_libbacktrace_path STREQUAL "libbacktrace.a"
            AND EXISTS "${_eggs_stacktrace_libbacktrace_path}"
        )
            set(EGGS_STACKTRACE_LIBBACKTRACE_LIBRARY
                "${_eggs_stacktrace_libbacktrace_path}"
                CACHE FILEPATH
                "Path to libbacktrace"
                FORCE
            )
        endif()
        unset(_eggs_stacktrace_libbacktrace_path)
    endif()
    # Initialized unconditionally: find_file below only runs when the
    # library was found, and this var must still be defined (as empty)
    # otherwise.
    set(EGGS_STACKTRACE_LIBBACKTRACE_HEADER)
    if(EGGS_STACKTRACE_LIBBACKTRACE_LIBRARY)
        # GCC-bundled copies put the header next to the library in an
        # include/ subdirectory; standalone installs use the normal include
        # hierarchy. Resolved to the header file itself (not just its
        # directory) so the caller can #include it by absolute path,
        # instead of adding its directory to the search path - which, for
        # a GCC-bundled copy, sits alongside GCC's own private freestanding
        # C headers (stddef.h, stdarg.h, ...) and would shadow the C++
        # standard library's own copies when compiling with e.g. Clang
        # -stdlib=libc++.
        cmake_path(
            GET EGGS_STACKTRACE_LIBBACKTRACE_LIBRARY
            PARENT_PATH _eggs_stacktrace_libbacktrace_libdir
        )
        find_file(
            EGGS_STACKTRACE_LIBBACKTRACE_HEADER
            NAMES backtrace.h
            HINTS
                "${_eggs_stacktrace_libbacktrace_libdir}/include"
                "${_eggs_stacktrace_libbacktrace_libdir}/../include"
        )
        unset(_eggs_stacktrace_libbacktrace_libdir)
    endif()
    mark_as_advanced(
        EGGS_STACKTRACE_LIBBACKTRACE_LIBRARY
        EGGS_STACKTRACE_LIBBACKTRACE_HEADER
    )

    if(
        EGGS_STACKTRACE_LIBBACKTRACE_LIBRARY
        AND EGGS_STACKTRACE_LIBBACKTRACE_HEADER
    )
        set(${out_var} TRUE PARENT_SCOPE)
        if(NOT TARGET _eggs_stacktrace_libbacktrace_support)
            add_library(_eggs_stacktrace_libbacktrace_support INTERFACE)
            target_link_libraries(
                _eggs_stacktrace_libbacktrace_support
                INTERFACE ${EGGS_STACKTRACE_LIBBACKTRACE_LIBRARY}
            )
        endif()
    else()
        set(${out_var} FALSE PARENT_SCOPE)
    endif()
endfunction()
