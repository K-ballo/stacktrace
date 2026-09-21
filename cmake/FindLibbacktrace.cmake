# Eggs.Stacktrace
#
# Copyright (c) 2026 Agustin Berge
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#[=======================================================================[.rst:
FindLibbacktrace
----------------

Locates libbacktrace (Ian Lance Taylor, distributed with GCC), which ships
either as a standalone install or bundled inside a GCC compiler
installation. Tries the standalone install first, then asks the compiler
where its private copy lives; ``-print-file-name`` echoes the bare name back
unchanged when absent.

Imported Targets
^^^^^^^^^^^^^^^^^

``Libbacktrace::Libbacktrace``
  The library, if found. Link-only: deliberately carries no
  INTERFACE_INCLUDE_DIRECTORIES, see ``Libbacktrace_HEADER`` below.

Result Variables
^^^^^^^^^^^^^^^^^

``Libbacktrace_FOUND``
  True if libbacktrace was found.
``Libbacktrace_LIBRARY``
  Path to the library.
``Libbacktrace_HEADER``
  Absolute path to the resolved backtrace.h. Deliberately NOT exposed as an
  include directory: a GCC-bundled copy sits alongside GCC's own private
  freestanding C headers (stddef.h, stdarg.h, ...) and would shadow the C++
  standard library's own copies when compiling with e.g. Clang
  -stdlib=libc++. Consumers should #include it by absolute path instead
  (see EGGS_STACKTRACE_BACKTRACE_INCLUDE_FILE in CheckLibbacktraceBackend.cmake).
#]=======================================================================]

find_library(Libbacktrace_LIBRARY NAMES backtrace)
if(NOT Libbacktrace_LIBRARY AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    execute_process(
        COMMAND "${CMAKE_CXX_COMPILER}" -print-file-name=libbacktrace.a
        OUTPUT_VARIABLE _libbacktrace_path
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(
        _libbacktrace_path
        AND NOT _libbacktrace_path STREQUAL "libbacktrace.a"
        AND EXISTS "${_libbacktrace_path}"
    )
        set(Libbacktrace_LIBRARY
            "${_libbacktrace_path}"
            CACHE FILEPATH
            "Path to libbacktrace"
            FORCE
        )
    endif()
    unset(_libbacktrace_path)
endif()

# Initialized unconditionally: find_file below only runs when the library
# was found, and this var must still be defined (as empty) otherwise.
set(Libbacktrace_HEADER)
if(Libbacktrace_LIBRARY)
    # GCC-bundled copies put the header next to the library in an include/
    # subdirectory; standalone installs use the normal include hierarchy.
    cmake_path(GET Libbacktrace_LIBRARY PARENT_PATH _libbacktrace_libdir)
    find_file(
        Libbacktrace_HEADER
        NAMES backtrace.h
        HINTS
            "${_libbacktrace_libdir}/include"
            "${_libbacktrace_libdir}/../include"
    )
    unset(_libbacktrace_libdir)
endif()
mark_as_advanced(Libbacktrace_LIBRARY Libbacktrace_HEADER)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    Libbacktrace
    REQUIRED_VARS Libbacktrace_LIBRARY Libbacktrace_HEADER
)

if(Libbacktrace_FOUND AND NOT TARGET Libbacktrace::Libbacktrace)
    add_library(Libbacktrace::Libbacktrace INTERFACE IMPORTED GLOBAL)
    target_link_libraries(
        Libbacktrace::Libbacktrace
        INTERFACE ${Libbacktrace_LIBRARY}
    )
endif()
