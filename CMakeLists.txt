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
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
        "$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>"
    )

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

#
# Library targets
#

# -- Exported names for each backend ------------------------------------------

set(EGGS_STACKTRACE_BACKEND_EXPORT_null Null)
set(EGGS_STACKTRACE_BACKEND_EXPORT_libbacktrace Libbacktrace)
set(EGGS_STACKTRACE_BACKEND_EXPORT_execinfo Execinfo)

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
                include/eggs/stacktrace/detail/nodiscard.hpp
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

# -- Backend macro -------------------------------------------------------------

set(EGGS_STACKTRACE_BACKENDS)

macro(eggs_stacktrace_add_backend name)
    list(APPEND EGGS_STACKTRACE_BACKENDS ${name})
    set(_export "${EGGS_STACKTRACE_BACKEND_EXPORT_${name}}")
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
    if(NOT "${ARGN}" STREQUAL "")
        target_link_libraries(_eggs_stacktrace_${name} PUBLIC ${ARGN})
    endif()
    add_library(Eggs::Stacktrace::${_export} ALIAS _eggs_stacktrace_${name})
    unset(_export)
endmacro()

# -- Backend targets -----------------------------------------------------------

eggs_stacktrace_add_backend(null)

include(CheckLibbacktraceBackend)
eggs_stacktrace_check_libbacktrace_backend(EGGS_STACKTRACE_HAVE_LIBBACKTRACE)
if(EGGS_STACKTRACE_HAVE_LIBBACKTRACE)
    eggs_stacktrace_add_backend(
        libbacktrace
        _eggs_stacktrace_libbacktrace_support
    )
    # #include'd by absolute path (see stacktrace_libbacktrace.cpp) instead
    # of adding its directory to the search path. PRIVATE: only this
    # translation unit needs it, unlike the support library's link
    # requirement above, which must stay PUBLIC for consumers.
    target_compile_definitions(
        _eggs_stacktrace_libbacktrace
        PRIVATE
            EGGS_STACKTRACE_BACKTRACE_INCLUDE_FILE="${EGGS_STACKTRACE_LIBBACKTRACE_HEADER}"
    )
endif()

include(CheckExecinfoBackend)
eggs_stacktrace_check_execinfo_backend(EGGS_STACKTRACE_HAVE_EXECINFO)
if(EGGS_STACKTRACE_HAVE_EXECINFO)
    eggs_stacktrace_add_backend(execinfo _eggs_stacktrace_execinfo_support)
endif()

# -- Main alias ----------------------------------------------------------------
# Note: ALIAS cannot alias another ALIAS, so we alias the internal target name.
# libbacktrace is preferred over execinfo: it additionally resolves source
# file/line, which execinfo cannot provide.

if(TARGET _eggs_stacktrace_libbacktrace)
    set(EGGS_STACKTRACE_DEFAULT_BACKEND libbacktrace)
elseif(TARGET _eggs_stacktrace_execinfo)
    set(EGGS_STACKTRACE_DEFAULT_BACKEND execinfo)
else()
    set(EGGS_STACKTRACE_DEFAULT_BACKEND null)
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

    macro(eggs_stacktrace_install_backend name)
        if(TARGET _eggs_stacktrace_${name})
            install(
                TARGETS _eggs_stacktrace_${name}
                EXPORT EggsStacktraceTargets
                ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            )
            # Referenced via the backend's PUBLIC link libraries; must be
            # part of the same export set.
            if(TARGET _eggs_stacktrace_${name}_support)
                install(
                    TARGETS _eggs_stacktrace_${name}_support
                    EXPORT EggsStacktraceTargets
                )
            endif()
            if(MSVC)
                install(
                    FILES
                        "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/eggs_stacktrace_${name}.pdb"
                    DESTINATION ${CMAKE_INSTALL_LIBDIR}
                    CONFIGURATIONS Debug RelWithDebInfo
                    OPTIONAL
                )
            endif()
        endif()
    endmacro()

    eggs_stacktrace_install_backend(null)
    eggs_stacktrace_install_backend(libbacktrace)
    eggs_stacktrace_install_backend(execinfo)

    install(
        EXPORT EggsStacktraceTargets
        NAMESPACE Eggs::
        FILE eggs.stacktrace-targets.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/eggs.stacktrace
    )

    install(
        FILES
            cmake/CheckLibbacktraceBackend.cmake
            cmake/CheckExecinfoBackend.cmake
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
