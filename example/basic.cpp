// Eggs.Stacktrace
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

// Demonstrates:
//   - a real call chain that exercises three symbol-resolution scenarios:
//     an anonymous-namespace function (internal linkage), a plain namespaced
//     function, and a template instantiation
//   - Section 1: the full trace as a formatted string
//   - Section 2: a single entry via its default format
//   - Section 3: the top entry's fields queried individually

#include <eggs/stacktrace.hpp>

#include <iostream>

// EGGS_STACKTRACE_NOINLINE - suppresses inlining on a function definition.
#ifndef EGGS_STACKTRACE_NOINLINE
#    ifdef _MSC_VER
#        define EGGS_STACKTRACE_NOINLINE __declspec(noinline)
#    else
#        define EGGS_STACKTRACE_NOINLINE [[gnu::noinline]]
#    endif
#endif

#if __cplusplus >= 202002L
#    include <version>
#endif
#ifdef __cpp_lib_format
#    include <format>
#endif

// -- Call chain ----------------------------------------------------------------

namespace example {

namespace {

// Pinned frames - the compiler is not allowed to merge these into callers.

EGGS_STACKTRACE_NOINLINE eggs::stacktrace innermost()
{
    return eggs::stacktrace::current();
}

// Not pinned: may be inlined into middle() under optimisation.
// Debug: visible as a distinct (anonymous namespace)::adapt frame.
// Release: typically absent - merged into middle().
eggs::stacktrace adapt()
{
    return innermost();
}

} // namespace

EGGS_STACKTRACE_NOINLINE eggs::stacktrace middle()
{
    return adapt();
}

// Not pinned: may be inlined into outer<42>() under optimisation.
// Debug: visible as example::dispatch.
// Release: typically absent - merged into outer<42>().
eggs::stacktrace dispatch()
{
    return middle();
}

template <int N>
EGGS_STACKTRACE_NOINLINE eggs::stacktrace outer()
{
    return dispatch();
}

} // namespace example

// -- main ----------------------------------------------------------------------

int main()
{
    auto const st = example::outer<42>();

    // -- Section 1: full trace via built-in formatting -------------------------

    std::cout << "=== full trace ===\n";
#ifdef __cpp_lib_format
    std::cout << std::format("{}", st);
#else
    std::cout << st;
#endif

    if (st.empty()) {
        std::cout << "\n(no frames captured; skipping single-entry sections)\n";
        return 0;
    }

    // -- Section 2: single entry via built-in formatting ------------------------

    std::cout << "\n=== single entry (default format) ===\n";
#ifdef __cpp_lib_format
    std::cout << std::format("{}\n", st[0]);
#else
    std::cout << st[0] << "\n";
#endif

    // -- Section 3: top entry's fields queried individually ----------------------
    // Source info is only populated in debug builds (-g / /Zi).

    eggs::stacktrace_entry const& top = st[0];
    std::cout << "\n=== top frame fields ===\n";
    std::cout << "native_handle: " << top.native_handle() << "\n";
    std::cout << "description:   " << top.description() << "\n";
    std::cout << "source_file:   " << top.source_file() << "\n";
    std::cout << "source_line:   " << top.source_line() << "\n";
}
