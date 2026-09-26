// Eggs.Stacktrace
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#include <type_traits>

#include "support.hpp"

#include <stdeggs/stacktrace.hpp>

#if defined(__cpp_lib_stacktrace) && !defined(EGGS_STACKTRACE_STD_USE_EGGS)
#    include <stacktrace>
#else
#    include <eggs/stacktrace.hpp>
#endif

int main()
{
    // -- Aliases ---------------------------------------------------------------

#if defined(__cpp_lib_stacktrace) && !defined(EGGS_STACKTRACE_STD_USE_EGGS)
    static_assert(
        std::is_same<stdeggs::stacktrace_entry, std::stacktrace_entry>::value,
        "stdeggs::stacktrace_entry is std::stacktrace_entry"
    );
    static_assert(
        std::is_same<stdeggs::stacktrace, std::stacktrace>::value,
        "stdeggs::stacktrace is std::stacktrace"
    );
#else
    static_assert(
        std::is_same<stdeggs::stacktrace_entry, eggs::stacktrace_entry>::value,
        "stdeggs::stacktrace_entry is eggs::stacktrace_entry"
    );
    static_assert(
        std::is_same<stdeggs::stacktrace, eggs::stacktrace>::value,
        "stdeggs::stacktrace is eggs::stacktrace"
    );
#endif

    // -- Default construction --------------------------------------------------

    stdeggs::stacktrace_entry const e;
    EGGS_STACKTRACE_CHECK(!e);

    stdeggs::stacktrace const st;
    EGGS_STACKTRACE_CHECK(st.empty());

    // -- Capture ---------------------------------------------------------------

    // Checks that the target provides whatever capture needs to link.
    stdeggs::stacktrace const cur = stdeggs::stacktrace::current();
    EGGS_STACKTRACE_CHECK(cur.size() <= cur.max_size());

    return eggs::test_support::report();
}
