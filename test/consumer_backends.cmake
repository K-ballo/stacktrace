# Eggs.Stacktrace
#
# Copyright (c) 2026 Agustin Berge
#
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt

include_guard()

# Resolves <target> through any ALIAS chain, storing the real target name.
function(_eggs_stacktrace_resolve_alias out_var target)
    get_target_property(_real ${target} ALIASED_TARGET)
    if(NOT _real)
        set(_real ${target})
    endif()
    set(${out_var} ${_real} PARENT_SCOPE)
endfunction()

# eggs_stacktrace_check_expected_backends()
#
# Verifies that the Eggs.Stacktrace targets in scope provide exactly the
# backends the platform is expected to support.
#
#   EGGS_EXPECTED_BACKENDS         list of export names, e.g. "Null;Execinfo"
#   EGGS_EXPECTED_DEFAULT_BACKEND  export name Eggs::Stacktrace should alias
#
# Does nothing if EGGS_EXPECTED_BACKENDS is not defined.
function(eggs_stacktrace_check_expected_backends)
    if(NOT DEFINED EGGS_EXPECTED_BACKENDS)
        return()
    endif()

    foreach(_export IN LISTS EGGS_EXPECTED_BACKENDS)
        if(NOT TARGET Eggs::Stacktrace::${_export})
            message(
                SEND_ERROR
                "Eggs.Stacktrace: expected backend target Eggs::Stacktrace::${_export} is not available"
            )
        endif()
    endforeach()

    if(
        DEFINED EGGS_EXPECTED_DEFAULT_BACKEND
        AND NOT TARGET Eggs::Stacktrace::${EGGS_EXPECTED_DEFAULT_BACKEND}
    )
        message(
            SEND_ERROR
            "Eggs.Stacktrace: expected default backend target Eggs::Stacktrace::${EGGS_EXPECTED_DEFAULT_BACKEND} is not available"
        )
    elseif(DEFINED EGGS_EXPECTED_DEFAULT_BACKEND)
        _eggs_stacktrace_resolve_alias(_actual Eggs::Stacktrace)
        _eggs_stacktrace_resolve_alias(
            _expected
            Eggs::Stacktrace::${EGGS_EXPECTED_DEFAULT_BACKEND}
        )
        if(NOT _actual STREQUAL _expected)
            message(
                SEND_ERROR
                "Eggs.Stacktrace: Eggs::Stacktrace resolves to '${_actual}', expected the ${EGGS_EXPECTED_DEFAULT_BACKEND} backend ('${_expected}')"
            )
        endif()
    endif()
endfunction()

# eggs_stacktrace_verify_backends()
#
# Runs the check above, then, for every expected backend, builds and registers
# a test of a consumer linked to just that backend.
function(eggs_stacktrace_verify_backends)
    eggs_stacktrace_check_expected_backends()

    foreach(_export IN LISTS EGGS_EXPECTED_BACKENDS)
        if(NOT TARGET Eggs::Stacktrace::${_export})
            continue()
        endif()

        string(TOLOWER "${_export}" _backend)
        add_executable(consumer_verify_${_backend} main.cpp)
        target_link_libraries(
            consumer_verify_${_backend}
            PRIVATE Eggs::Stacktrace::${_export}
        )
        add_test(NAME verify_${_backend} COMMAND consumer_verify_${_backend})
    endforeach()
endfunction()
