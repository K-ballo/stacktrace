# Eggs.Stacktrace
#
# Copyright (c) 2026 Agustin Berge
#
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt

# -- Verify interface header sets ----------------------------------------------

add_test(
    NAME eggs.stacktrace.verify_headers
    COMMAND
        ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target
        _eggs_stacktrace_common_verify_interface_header_sets
        _eggs_stacktrace_std_verify_interface_header_sets --config $<CONFIG>
)
set_tests_properties(
    eggs.stacktrace.verify_headers
    PROPERTIES LABELS "header" RESOURCE_LOCK cmake_build
)

# -- Verify definitions in headers -------------------------------------------

include(AddMultipleDefinitionsTest)

foreach(_backend IN LISTS EGGS_STACKTRACE_BACKENDS)
    add_multiple_definitions_test(
        eggs.stacktrace.verify_definitions.${_backend}
        _eggs_stacktrace_common
        _eggs_stacktrace_${_backend}
    )
    set_target_properties(
        eggs.stacktrace.verify_definitions.${_backend}
        PROPERTIES FOLDER "header"
    )
    set_tests_properties(
        eggs.stacktrace.verify_definitions.${_backend}
        PROPERTIES LABELS "header" RESOURCE_LOCK cmake_build
    )
endforeach()

add_multiple_definitions_test(
    eggs.stacktrace.verify_definitions.std
    _eggs_stacktrace_common
    _eggs_stacktrace_std
)
set_target_properties(
    eggs.stacktrace.verify_definitions.std
    PROPERTIES FOLDER "header"
)
set_tests_properties(
    eggs.stacktrace.verify_definitions.std
    PROPERTIES LABELS "header" RESOURCE_LOCK cmake_build
)

# Helper - creates a test executable and registers it with CTest.
function(add_lib_test name)
    add_executable(eggs.stacktrace.test.${name} ${name}.cpp)
    target_sources(
        eggs.stacktrace.test.${name}
        PRIVATE
            FILE_SET private_headers
                TYPE HEADERS
                BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
                FILES support.hpp
    )
    target_link_libraries(
        eggs.stacktrace.test.${name}
        PRIVATE Eggs::Stacktrace::Null
    )

    add_test(
        NAME eggs.stacktrace.test.${name}
        COMMAND eggs.stacktrace.test.${name}
    )
    set_tests_properties(eggs.stacktrace.test.${name} PROPERTIES LABELS "unit")
endfunction()

# -- Unit tests ----------------------------------------------------------------

add_lib_test(stacktrace_entry)
add_lib_test(stacktrace)
add_lib_test(stacktrace_formatter)

# -- stdeggs tests ---------------------------------------------------------------

# Helper - creates a stdeggs test executable, with optional compile definitions.
function(add_stdeggs_test name)
    add_executable(eggs.stacktrace.test.${name} stdeggs.cpp)
    target_sources(
        eggs.stacktrace.test.${name}
        PRIVATE
            FILE_SET private_headers
                TYPE HEADERS
                BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
                FILES support.hpp
    )
    target_link_libraries(
        eggs.stacktrace.test.${name}
        PRIVATE Eggs::Stacktrace::Std
    )
    target_compile_definitions(eggs.stacktrace.test.${name} PRIVATE ${ARGN})

    add_test(
        NAME eggs.stacktrace.test.${name}
        COMMAND eggs.stacktrace.test.${name}
    )
    set_tests_properties(eggs.stacktrace.test.${name} PROPERTIES LABELS "unit")
endfunction()

add_stdeggs_test(stdeggs)
add_stdeggs_test(stdeggs_use_eggs EGGS_STACKTRACE_STD_USE_EGGS)

# -- libbacktrace backend test ---------------------------------------------------

if(TARGET Eggs::Stacktrace::Libbacktrace)
    add_executable(
        eggs.stacktrace.test.stacktrace_libbacktrace
        stacktrace_libbacktrace.cpp
    )
    target_sources(
        eggs.stacktrace.test.stacktrace_libbacktrace
        PRIVATE
            FILE_SET private_headers
                TYPE HEADERS
                BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
                FILES support.hpp
    )
    target_link_libraries(
        eggs.stacktrace.test.stacktrace_libbacktrace
        PRIVATE Eggs::Stacktrace::Libbacktrace
    )
    add_test(
        NAME eggs.stacktrace.stacktrace_libbacktrace
        COMMAND eggs.stacktrace.test.stacktrace_libbacktrace
    )
    set_tests_properties(
        eggs.stacktrace.stacktrace_libbacktrace
        PROPERTIES LABELS "unit"
    )
endif()

# -- execinfo backend test ------------------------------------------------------

if(TARGET Eggs::Stacktrace::Execinfo)
    add_executable(
        eggs.stacktrace.test.stacktrace_execinfo
        stacktrace_execinfo.cpp
    )
    target_sources(
        eggs.stacktrace.test.stacktrace_execinfo
        PRIVATE
            FILE_SET private_headers
                TYPE HEADERS
                BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
                FILES support.hpp
    )
    target_link_libraries(
        eggs.stacktrace.test.stacktrace_execinfo
        PRIVATE Eggs::Stacktrace::Execinfo
    )
    set_target_properties(
        eggs.stacktrace.test.stacktrace_execinfo
        PROPERTIES ENABLE_EXPORTS ON
    )
    add_test(
        NAME eggs.stacktrace.stacktrace_execinfo
        COMMAND eggs.stacktrace.test.stacktrace_execinfo
    )
    set_tests_properties(
        eggs.stacktrace.stacktrace_execinfo
        PROPERTIES LABELS "unit"
    )
endif()

# -- win32 backend test ---------------------------------------------------------

if(TARGET Eggs::Stacktrace::Win32)
    add_executable(eggs.stacktrace.test.stacktrace_win32 stacktrace_win32.cpp)
    target_sources(
        eggs.stacktrace.test.stacktrace_win32
        PRIVATE
            FILE_SET private_headers
                TYPE HEADERS
                BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
                FILES support.hpp
    )
    target_link_libraries(
        eggs.stacktrace.test.stacktrace_win32
        PRIVATE Eggs::Stacktrace::Win32
    )
    add_test(
        NAME eggs.stacktrace.stacktrace_win32
        COMMAND eggs.stacktrace.test.stacktrace_win32
    )
    set_tests_properties(
        eggs.stacktrace.stacktrace_win32
        PROPERTIES LABELS "unit"
    )
endif()
