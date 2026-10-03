# Eggs.Stacktrace
#
# Copyright (c) 2026 Agustin Berge
#
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt

cmake_minimum_required(VERSION 4.0...4.4.0)
project(
    Eggs.Stacktrace
    VERSION 0.1.0
    DESCRIPTION "A C++ stacktrace library"
    LANGUAGES CXX
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(GNUInstallDirs)

# Options (two spellings: short when top-level, prefixed when a subdirectory)
if(PROJECT_IS_TOP_LEVEL)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT ProgramDatabase)
    if(MSVC)
        string(APPEND CMAKE_EXE_LINKER_FLAGS " /DEBUG")
        string(APPEND CMAKE_SHARED_LINKER_FLAGS " /DEBUG")
    endif()

    option(BUILD_EXAMPLES "Build the examples" ON)
    include(CTest)
    option(ENABLE_INSTALL "Generate install rules" ON)
else()
    set_property(DIRECTORY PROPERTY SYSTEM ON)

    option(EGGS_STACKTRACE_BUILD_EXAMPLES "Build the examples" OFF)
    option(EGGS_STACKTRACE_BUILD_TESTING "Build the test suite" OFF)
    option(EGGS_STACKTRACE_ENABLE_INSTALL "Generate install rules" OFF)

    set(BUILD_EXAMPLES ${EGGS_STACKTRACE_BUILD_EXAMPLES})
    set(BUILD_TESTING ${EGGS_STACKTRACE_BUILD_TESTING})
    set(ENABLE_INSTALL ${EGGS_STACKTRACE_ENABLE_INSTALL})
endif()

set(EGGS_STACKTRACE_KNOWN_BACKENDS null execinfo libbacktrace win32)

set(EGGS_STACKTRACE_BACKENDS
    ""
    CACHE STRING
    "List of backends to build: ${EGGS_STACKTRACE_KNOWN_BACKENDS}. \
Empty (default) auto-detects available backends."
)
foreach(_backend IN LISTS EGGS_STACKTRACE_BACKENDS)
    if(NOT _backend IN_LIST EGGS_STACKTRACE_KNOWN_BACKENDS)
        message(
            FATAL_ERROR
            "Eggs.Stacktrace: unknown backend '${_backend}' in "
            "EGGS_STACKTRACE_BACKENDS (known backends: "
            "${EGGS_STACKTRACE_KNOWN_BACKENDS})"
        )
    endif()
endforeach()
unset(_backend)

set(EGGS_STACKTRACE_DEFAULT_BACKEND
    ""
    CACHE STRING
    "Backend aliased as Eggs::Stacktrace: ${EGGS_STACKTRACE_KNOWN_BACKENDS}. \
Empty (default) auto-select best available backend."
)
set_property(
    CACHE EGGS_STACKTRACE_DEFAULT_BACKEND
    PROPERTY STRINGS "" ${EGGS_STACKTRACE_KNOWN_BACKENDS}
)
if(
    EGGS_STACKTRACE_DEFAULT_BACKEND
    AND NOT EGGS_STACKTRACE_DEFAULT_BACKEND IN_LIST EGGS_STACKTRACE_KNOWN_BACKENDS
)
    message(
        FATAL_ERROR
        "Eggs.Stacktrace: unknown backend "
        "'${EGGS_STACKTRACE_DEFAULT_BACKEND}' in "
        "EGGS_STACKTRACE_DEFAULT_BACKEND (known backends: "
        "${EGGS_STACKTRACE_KNOWN_BACKENDS})"
    )
endif()

#
# Library targets
#

# -- Exported names for each backend ------------------------------------------

set(EGGS_STACKTRACE_BACKEND_EXPORT_null Null)
set(EGGS_STACKTRACE_BACKEND_EXPORT_execinfo Execinfo)
set(EGGS_STACKTRACE_BACKEND_EXPORT_libbacktrace Libbacktrace)
set(EGGS_STACKTRACE_BACKEND_EXPORT_win32 Win32)

# -- Common OBJECT library ----

add_library(_eggs_stacktrace_common OBJECT src/stacktrace.cpp)
target_include_directories(
    _eggs_stacktrace_common
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(_eggs_stacktrace_common PUBLIC cxx_std_11)
target_sources(
    _eggs_stacktrace_common
    PUBLIC
        FILE_SET HEADERS
            BASE_DIRS include
            FILES
                include/eggs/stacktrace.hpp
                include/eggs/stacktrace/pin_frame.hpp
                include/eggs/stacktrace/detail/no_tail_calls.hpp
                include/eggs/stacktrace/detail/nodiscard.hpp
                include/eggs/stacktrace/detail/noinline.hpp
    PRIVATE
        FILE_SET private_headers
            TYPE HEADERS
            BASE_DIRS src
            FILES src/detail/asan_backtrace.hpp
            FILES src/detail/backend.hpp
)
set_target_properties(
    _eggs_stacktrace_common
    PROPERTIES VERIFY_INTERFACE_HEADER_SETS ${BUILD_TESTING}
)
if(MSVC)
    set_target_properties(
        _eggs_stacktrace_common
        PROPERTIES
            COMPILE_PDB_NAME "_eggs_stacktrace_common"
            COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>"
    )
endif()

# -- Backend helpers -------------------------------------------------------

# eggs_stacktrace_add_backend(<name>)
#
# Defines the _eggs_stacktrace_<name> target.
function(eggs_stacktrace_add_backend name)
    set(_export "${EGGS_STACKTRACE_BACKEND_EXPORT_${name}}")
    string(TOUPPER "${name}" _upper)
    add_library(
        _eggs_stacktrace_${name}
        STATIC
        $<TARGET_OBJECTS:_eggs_stacktrace_common>
        src/stacktrace_${name}.cpp
    )
    target_include_directories(
        _eggs_stacktrace_${name}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )
    target_compile_features(_eggs_stacktrace_${name} PUBLIC cxx_std_11)
    if(EGGS_STACKTRACE_${_upper}_DEFINITIONS)
        target_compile_definitions(
            _eggs_stacktrace_${name}
            PRIVATE ${EGGS_STACKTRACE_${_upper}_DEFINITIONS}
        )
    endif()
    set_target_properties(
        _eggs_stacktrace_${name}
        PROPERTIES
            EXPORT_NAME Stacktrace::${_export}
            OUTPUT_NAME eggs_stacktrace_${name}
            DEBUG_POSTFIX d
    )
    if(MSVC)
        set_target_properties(
            _eggs_stacktrace_${name}
            PROPERTIES
                COMPILE_PDB_NAME "eggs_stacktrace_${name}"
                COMPILE_PDB_OUTPUT_DIRECTORY
                    "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>"
        )
    endif()
    if(TARGET _eggs_stacktrace_${name}_support)
        target_link_libraries(
            _eggs_stacktrace_${name}
            PUBLIC _eggs_stacktrace_${name}_support
        )
    endif()
    add_library(Eggs::Stacktrace::${_export} ALIAS _eggs_stacktrace_${name})
endfunction()

# eggs_stacktrace_add_checked_backend(<name>)
#
# Adds backend <name> if it is wanted and its dependencies check out.
macro(eggs_stacktrace_add_checked_backend name)
    string(TOUPPER "${name}" _upper)
    if(
        NOT EGGS_STACKTRACE_BACKENDS
        OR "${name}" IN_LIST EGGS_STACKTRACE_BACKENDS
    )
        include(Check${EGGS_STACKTRACE_BACKEND_EXPORT_${name}}Backend)
        cmake_language(
            CALL eggs_stacktrace_check_${name}_backend
            EGGS_STACKTRACE_HAVE_${_upper}
        )
        if(EGGS_STACKTRACE_HAVE_${_upper})
            eggs_stacktrace_add_backend(${name})
            list(
                APPEND EGGS_STACKTRACE_CHECKED_BACKENDS
                ${EGGS_STACKTRACE_BACKEND_EXPORT_${name}}
            )
        elseif("${name}" IN_LIST EGGS_STACKTRACE_BACKENDS)
            message(
                FATAL_ERROR
                "Eggs.Stacktrace: backend '${name}' was requested in "
                "EGGS_STACKTRACE_BACKENDS but is not available on this platform."
            )
        endif()
    endif()
    unset(_upper)
endmacro()

# -- Backend targets -----------------------------------------------------------

set(EGGS_STACKTRACE_CHECKED_BACKENDS)
eggs_stacktrace_add_backend(null)
eggs_stacktrace_add_checked_backend(execinfo)
eggs_stacktrace_add_checked_backend(libbacktrace)
eggs_stacktrace_add_checked_backend(win32)

# An empty EGGS_STACKTRACE_BACKENDS means auto-detect.
if(NOT EGGS_STACKTRACE_BACKENDS)
    set(_detected)
    foreach(_backend IN LISTS EGGS_STACKTRACE_KNOWN_BACKENDS)
        if(TARGET _eggs_stacktrace_${_backend})
            list(APPEND _detected ${_backend})
        endif()
    endforeach()
    set_property(CACHE EGGS_STACKTRACE_BACKENDS PROPERTY VALUE "${_detected}")
    unset(_backend)
    unset(_detected)
endif()

# -- Main alias ----------------------------------------------------------------
# Note: ALIAS cannot alias another ALIAS, so we alias the internal target name.

if(EGGS_STACKTRACE_DEFAULT_BACKEND)
    if(NOT TARGET _eggs_stacktrace_${EGGS_STACKTRACE_DEFAULT_BACKEND})
        message(
            FATAL_ERROR
            "Eggs.Stacktrace: EGGS_STACKTRACE_DEFAULT_BACKEND requested "
            "'${EGGS_STACKTRACE_DEFAULT_BACKEND}', but that backend was not "
            "built (see EGGS_STACKTRACE_BACKENDS)."
        )
    endif()
else()
    set(EGGS_STACKTRACE_DEFAULT_BACKEND null)
    foreach(_backend IN ITEMS libbacktrace execinfo win32)
        if(TARGET _eggs_stacktrace_${_backend})
            set(EGGS_STACKTRACE_DEFAULT_BACKEND ${_backend})
            break()
        endif()
    endforeach()
    unset(_backend)
endif()

add_library(
    Eggs::Stacktrace
    ALIAS _eggs_stacktrace_${EGGS_STACKTRACE_DEFAULT_BACKEND}
)

#
# Examples
#
if(BUILD_EXAMPLES)
    add_subdirectory(example)
endif()

#
# Tests
#
if(BUILD_TESTING)
    add_subdirectory(test)
endif()

#
# Install rules
#
if(ENABLE_INSTALL)
    include(CMakePackageConfigHelpers)

    # Install public headers from the common OBJECT library's file set.
    install(
        TARGETS _eggs_stacktrace_common
        FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    # -- Backend install helper ------------------------------------------------

    function(eggs_stacktrace_install_backend name)
        if(NOT TARGET _eggs_stacktrace_${name})
            return()
        endif()

        set(_export_set EggsStacktraceTargets)
        set(_export_file eggs.stacktrace-targets.cmake)
        if(NOT "${name}" STREQUAL "null")
            set(_export_set
                "EggsStacktrace${EGGS_STACKTRACE_BACKEND_EXPORT_${name}}Targets"
            )
            set(_export_file "eggs.stacktrace-${name}-targets.cmake")
        endif()
        install(
            TARGETS _eggs_stacktrace_${name}
            EXPORT ${_export_set}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )
        if(TARGET _eggs_stacktrace_${name}_support)
            install(
                TARGETS _eggs_stacktrace_${name}_support
                EXPORT ${_export_set}
            )
        endif()
        if(MSVC)
            install(
                FILES
                    "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/eggs_stacktrace_${name}.pdb"
                DESTINATION ${CMAKE_INSTALL_LIBDIR}
                OPTIONAL
            )
        endif()
        install(
            EXPORT ${_export_set}
            NAMESPACE Eggs::
            FILE ${_export_file}
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/eggs.stacktrace
        )
    endfunction()

    eggs_stacktrace_install_backend(null)
    eggs_stacktrace_install_backend(execinfo)
    eggs_stacktrace_install_backend(libbacktrace)
    eggs_stacktrace_install_backend(win32)

    install(
        FILES
            cmake/CheckExecinfoBackend.cmake
            cmake/CheckLibbacktraceBackend.cmake
            cmake/CheckWin32Backend.cmake
            cmake/FindLibbacktrace.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/eggs.stacktrace
    )

    set(EGGS_STACKTRACE_DEFAULT_BACKEND_EXPORT
        "${EGGS_STACKTRACE_BACKEND_EXPORT_${EGGS_STACKTRACE_DEFAULT_BACKEND}}"
    )

    configure_package_config_file(
        cmake/eggs.stacktrace-config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/eggs.stacktrace-config.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/eggs.stacktrace
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/eggs.stacktrace-config-version.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/eggs.stacktrace-config.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/eggs.stacktrace-config-version.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/eggs.stacktrace
    )

    set(CPACK_PACKAGE_NAME "Eggs.Stacktrace")
    set(CPACK_PACKAGE_VENDOR "Agustin Berge")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
    if(WIN32)
        set(CPACK_GENERATOR ZIP)
    else()
        set(CPACK_GENERATOR TGZ)
    endif()
    include(CPack)
endif()
