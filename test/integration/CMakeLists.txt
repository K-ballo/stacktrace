# Eggs.Stacktrace
#
# Copyright (c) 2026 Agustin Berge
#
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt

# A representative consumer of an installed (Release) Eggs.Stacktrace package:
# a static library, a shared library, a runtime-loaded module and an app,
# built once per installed backend. Meant to be driven by run_matrix.py,
# which builds it under many flag combinations using the `Integration`
# configuration.

cmake_minimum_required(VERSION 3.25)
project(Eggs.Stacktrace-Integration LANGUAGES CXX)

if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

# Everything that runs lands in one directory, and finds its siblings
# relative to itself, so it can be copied elsewhere and post-processed.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)

# Only a Release package is installed; whatever configuration this is built
# with (the driver uses `Integration`) consumes it.
set(CMAKE_MAP_IMPORTED_CONFIG_INTEGRATION Release)

if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT _ipo_supported OUTPUT _ipo_output)
    if(NOT _ipo_supported)
        # Recognized by run_matrix.py, which skips rather than fails.
        message(FATAL_ERROR "INTEGRATION_UNSUPPORTED: ${_ipo_output}")
    endif()
endif()

find_package(Eggs.Stacktrace REQUIRED)
find_package(Threads REQUIRED)

# -- Backend independent -------------------------------------------------------

add_library(integration_context INTERFACE)
target_include_directories(
    integration_context
    INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

add_library(integration_plugin SHARED src/plugin.cpp)
target_link_libraries(integration_plugin PUBLIC integration_context)

add_library(integration_module MODULE src/module.cpp)
target_link_libraries(integration_module PRIVATE integration_context)

add_library(integration_module_b MODULE src/module_b.cpp)
target_link_libraries(integration_module_b PRIVATE integration_context)

# -- Per backend ---------------------------------------------------------------

set(_backends)
foreach(_backend IN ITEMS Null Execinfo Libbacktrace Win32)
    if(NOT TARGET Eggs::Stacktrace::${_backend})
        continue()
    endif()
    string(TOLOWER "${_backend}" _name)
    list(APPEND _backends ${_name})

    add_library(integration_core_${_name} STATIC src/core.cpp)
    target_link_libraries(
        integration_core_${_name}
        PUBLIC Eggs::Stacktrace::${_backend} integration_plugin Threads::Threads
    )

    add_executable(integration_app_${_name} src/main.cpp)
    target_compile_definitions(
        integration_app_${_name}
        PRIVATE
            INTEGRATION_BACKEND="${_name}"
            INTEGRATION_MODULE_NAME="$<TARGET_FILE_NAME:integration_module>"
            INTEGRATION_MODULE_B_NAME="$<TARGET_FILE_NAME:integration_module_b>"
    )
    target_link_libraries(
        integration_app_${_name}
        PRIVATE integration_core_${_name} ${CMAKE_DL_LIBS}
    )
    add_dependencies(
        integration_app_${_name}
        integration_module
        integration_module_b
    )
endforeach()

message(STATUS "Integration backends: ${_backends}")
