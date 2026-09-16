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

// __declspec(noinline) is understood by MSVC, clang-cl, and MinGW-GCC.
// No ENABLE_EXPORTS needed: SymFromAddr reads PDB symbols, not .dynsym.
__declspec(noinline) void eggs_stacktrace_win32_sentinel()
{
    auto st = eggs::stacktrace::current(0);
    EGGS_STACKTRACE_CHECK(!st.empty());

    // Frame 0 must be this function.
    std::string const desc = st[0].description();
    EGGS_STACKTRACE_CHECK(!desc.empty());
    EGGS_STACKTRACE_CHECK(
        desc.find("eggs_stacktrace_win32_sentinel") != std::string::npos
    );

    // DbgHelp provides PDB source info in Debug builds (with /Zi or /ZI).
    // Skip in Release/NDEBUG since the binary may lack a PDB.
#ifndef NDEBUG
    EGGS_STACKTRACE_CHECK(!st[0].source_file().empty());
    EGGS_STACKTRACE_CHECK(st[0].source_line() != 0u);
#endif
}

int main()
{
    // -- current() returns non-empty --------------------------------------------

    auto const st = eggs::stacktrace::current(0);
    EGGS_STACKTRACE_CHECK(!st.empty());

    // -- skip reduces frame count by exactly 1 ----------------------------------

    auto const st0 = eggs::stacktrace::current(0);
    auto const st1 = eggs::stacktrace::current(1);
    EGGS_STACKTRACE_CHECK(st1.size() + 1 == st0.size());

    // -- max_depth caps the result -----------------------------------------------

    auto const st_capped = eggs::stacktrace::current(0, 1);
    EGGS_STACKTRACE_CHECK(st_capped.size() == 1);

    // -- description and PDB source info ------------------------------------------

    eggs_stacktrace_win32_sentinel();

    return eggs::test_support::report();
}
