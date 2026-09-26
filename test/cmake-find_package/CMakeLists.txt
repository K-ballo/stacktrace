# Eggs.Stacktrace
#
# Copyright (c) 2026 Agustin Berge
#
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt

cmake_minimum_required(VERSION 3.21)
project(Eggs.Stacktrace-FindPackage LANGUAGES CXX)

set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
    "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>"
)

find_package(Eggs.Stacktrace REQUIRED)

# Check for a real backend.
get_target_property(_backend Eggs::Stacktrace ALIASED_TARGET)
if(_backend STREQUAL "Eggs::Stacktrace::Null")
    message(FATAL_ERROR "Eggs::Stacktrace resolved to the Null backend")
endif()
message(STATUS "Eggs::Stacktrace backend: ${_backend}")

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
    message(STATUS "Eggs::Stacktrace ${_prop}: ${_val}")
endforeach()

enable_testing()
add_test(NAME verify COMMAND consumer_verify)
# Printed by example/basic.cpp (copied as main.cpp) when the trace is empty.
set_tests_properties(
    verify
    PROPERTIES FAIL_REGULAR_EXPRESSION "no frames captured"
)
