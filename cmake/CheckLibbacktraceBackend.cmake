# Eggs.Stacktrace
#
# Copyright (c) 2026 Agustin Berge
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

include_guard()

# eggs_stacktrace_check_libbacktrace_backend(<library-out-var> <include-out-var>)
#
# Locates libbacktrace (Ian Lance Taylor, distributed with GCC), which ships
# either as a standalone install or bundled inside a GCC compiler
# installation. Tries the standalone install first, then asks the compiler
# where its private copy lives; -print-file-name echoes the bare name back
# unchanged when absent. Sets <library-out-var> and <include-out-var> in the
# caller's scope; both are empty if the library could not be found.
function(
    eggs_stacktrace_check_libbacktrace_backend
    library_out_var
    include_out_var
)
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
    # GCC-bundled copies put the header next to the library in an include/
    # subdirectory; standalone installs use the normal include hierarchy.
    if(EGGS_STACKTRACE_LIBBACKTRACE_LIBRARY)
        cmake_path(
            GET EGGS_STACKTRACE_LIBBACKTRACE_LIBRARY
            PARENT_PATH _eggs_stacktrace_libbacktrace_libdir
        )
        find_path(
            EGGS_STACKTRACE_LIBBACKTRACE_INCLUDE_DIR
            NAMES backtrace.h
            HINTS
                "${_eggs_stacktrace_libbacktrace_libdir}/include"
                "${_eggs_stacktrace_libbacktrace_libdir}/../include"
        )
        unset(_eggs_stacktrace_libbacktrace_libdir)
    endif()
    mark_as_advanced(
        EGGS_STACKTRACE_LIBBACKTRACE_LIBRARY
        EGGS_STACKTRACE_LIBBACKTRACE_INCLUDE_DIR
    )
    set(${library_out_var}
        "${EGGS_STACKTRACE_LIBBACKTRACE_LIBRARY}"
        PARENT_SCOPE
    )
    set(${include_out_var}
        "${EGGS_STACKTRACE_LIBBACKTRACE_INCLUDE_DIR}"
        PARENT_SCOPE
    )
endfunction()
