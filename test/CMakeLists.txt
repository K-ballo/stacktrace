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
        _eggs_stacktrace_common_verify_interface_header_sets --config $<CONFIG>
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
