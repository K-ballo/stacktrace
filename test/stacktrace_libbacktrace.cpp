// Eggs.Stacktrace
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#include <eggs/stacktrace.hpp>

#include <string>

#include "support.hpp"

// noinline ensures this function has its own frame in the trace.
// No ENABLE_EXPORTS or visibility("default") needed: backtrace_syminfo reads
// .symtab (static table), so hidden symbols are found without -rdynamic.
[[gnu::noinline]]
void eggs_stacktrace_libbacktrace_sentinel()
{
    auto st = eggs::stacktrace::current(0);
    if (!EGGS_STACKTRACE_CHECK(!st.empty())) {
        EGGS_STACKTRACE_TRACE(st.size());
        return;
    }

    // Frame 0 must be this function.
    std::string const desc = st[0].description();
    if (!EGGS_STACKTRACE_CHECK(!desc.empty())) {
        EGGS_STACKTRACE_TRACE(desc);
    }
    if (!EGGS_STACKTRACE_CHECK(
            desc.find("eggs_stacktrace_libbacktrace_sentinel") !=
            std::string::npos
        )) {
        EGGS_STACKTRACE_TRACE(desc);
    }

    // libbacktrace provides DWARF source info when debug symbols are present
    // (i.e., in a Debug build). Skip these checks in Release/NDEBUG builds
    // since the binary may be stripped.
#ifndef NDEBUG
    if (!EGGS_STACKTRACE_CHECK(!st[0].source_file().empty())) {
        EGGS_STACKTRACE_TRACE(st[0].source_file());
    }
    if (!EGGS_STACKTRACE_CHECK(st[0].source_line() != 0u)) {
        EGGS_STACKTRACE_TRACE(st[0].source_line());
    }
#endif
}

int main()
{
    // -- current() returns non-empty --------------------------------------------

    auto const st = eggs::stacktrace::current(0);
    if (!EGGS_STACKTRACE_CHECK(!st.empty())) {
        EGGS_STACKTRACE_TRACE(st.size());
    }

    // -- skip reduces frame count by exactly 1 ----------------------------------

    auto const st0 = eggs::stacktrace::current(0);
    auto const st1 = eggs::stacktrace::current(1);
    if (!EGGS_STACKTRACE_CHECK(st1.size() + 1 == st0.size())) {
        EGGS_STACKTRACE_TRACE(st0.size());
        EGGS_STACKTRACE_TRACE(st1.size());
    }

    // -- max_depth caps the result -----------------------------------------------

    auto const st_capped = eggs::stacktrace::current(0, 1);
    if (!EGGS_STACKTRACE_CHECK(st_capped.size() == 1)) {
        EGGS_STACKTRACE_TRACE(st_capped.size());
    }

    // -- description and DWARF source info ---------------------------------------

    eggs_stacktrace_libbacktrace_sentinel();

    return eggs::test_support::report();
}
