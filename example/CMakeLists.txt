# Eggs.Stacktrace
#
# Copyright (c) 2026 Agustin Berge
#
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt

function(add_example name)
    add_executable(eggs.stacktrace.example.${name} ${name}.cpp)
    target_link_libraries(eggs.stacktrace.example.${name} PRIVATE ${ARGN})

    if(BUILD_TESTING)
        add_test(
            NAME eggs.stacktrace.example.${name}
            COMMAND eggs.stacktrace.example.${name}
        )
        set_tests_properties(
            eggs.stacktrace.example.${name}
            PROPERTIES LABELS "example"
        )
    endif()
endfunction()

add_example(basic Eggs::Stacktrace)
add_example(exception Eggs::Stacktrace)
