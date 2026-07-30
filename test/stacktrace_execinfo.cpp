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

// C linkage + noinline + default visibility gives a predictable unmangled
// symbol that dladdr() can resolve via -rdynamic even under -fvisibility=hidden.
extern "C" [[gnu::noinline, gnu::visibility("default")]]
void eggs_stacktrace_execinfo_sentinel()
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
            desc.find("eggs_stacktrace_execinfo_sentinel") != std::string::npos
        )) {
        EGGS_STACKTRACE_TRACE(desc);
    }

    // execinfo provides no DWARF source info.
    if (!EGGS_STACKTRACE_CHECK(st[0].source_file() == "")) {
        EGGS_STACKTRACE_TRACE(st[0].source_file());
    }
    if (!EGGS_STACKTRACE_CHECK(st[0].source_line() == 0u)) {
        EGGS_STACKTRACE_TRACE(st[0].source_line());
    }
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

    // -- source_file and source_line are always empty / 0 ------------------------

    for (auto const& e : st) {
        if (!EGGS_STACKTRACE_CHECK(e.source_file() == "")) {
            EGGS_STACKTRACE_TRACE(e.source_file());
        }
        if (!EGGS_STACKTRACE_CHECK(e.source_line() == 0u)) {
            EGGS_STACKTRACE_TRACE(e.source_line());
        }
    }

    // -- description resolves via dladdr + ENABLE_EXPORTS ------------------------

    eggs_stacktrace_execinfo_sentinel();

    return eggs::test_support::report();
}
