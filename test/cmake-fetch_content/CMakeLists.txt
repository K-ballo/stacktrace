# Eggs.Stacktrace
#
# Copyright (c) 2026 Agustin Berge
#
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt

cmake_minimum_required(VERSION 4.0...4.4.0)
project(Eggs.Stacktrace-FetchContent LANGUAGES CXX)

set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
    "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>"
)

# EGGS_STACKTRACE_SOURCE_DIR is injected by CI to point at the local tree.
if(NOT DEFINED EGGS_STACKTRACE_SOURCE_DIR)
    message(
        FATAL_ERROR
        "EGGS_STACKTRACE_SOURCE_DIR must be set. "
        "Pass -DEGGS_STACKTRACE_SOURCE_DIR=<path> to cmake."
    )
endif()
file(TO_CMAKE_PATH "${EGGS_STACKTRACE_SOURCE_DIR}" EGGS_STACKTRACE_SOURCE_DIR)

include(FetchContent)
FetchContent_Declare(Eggs.Stacktrace SOURCE_DIR "${EGGS_STACKTRACE_SOURCE_DIR}")
FetchContent_MakeAvailable(Eggs.Stacktrace)

add_executable(consumer_verify main.cpp)
target_link_libraries(consumer_verify PRIVATE Eggs::Stacktrace)

foreach(
    _prop
    IN
    ITEMS
        INTERFACE_INCLUDE_DIRECTORIES
        INTERFACE_COMPILE_FEATURES
        INTERFACE_LINK_LIBRARIES
)
    get_target_property(_val Eggs::Stacktrace ${_prop})
    message(STATUS "Eggs::Stacktrace (FetchContent) ${_prop}: ${_val}")
endforeach()

enable_testing()
add_test(NAME verify COMMAND consumer_verify)
