// Eggs.Stacktrace
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <cstdio>
#include <cstdlib>

// A minimal, always-on check for lib tests, which must not depend on the
// library they are testing. Unlike an assert, it does not abort on
// failure, so it must not guard against undefined behavior in the code
// that follows.
//
// main() should end with `return eggs::test_support::report();`.

namespace eggs {
namespace test_support {

template <typename = void>
struct counters
{
    static int passed;
    static int failed;
};

template <typename T>
int counters<T>::passed = 0;
template <typename T>
int counters<T>::failed = 0;

inline int report()
{
    std::fprintf(
        stderr, "%d passed, %d failed\n", counters<>::passed, counters<>::failed
    );
    return counters<>::failed != 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}

inline void check(bool cond, char const* file, int line, char const* expr)
{
    if (cond) {
        ++counters<>::passed;
    } else {
        std::fprintf(stderr, "%s:%d: check failed: %s\n", file, line, expr);
        ++counters<>::failed;
    }
}

} // namespace test_support
} // namespace eggs

#define EGGS_STACKTRACE_CHECK(...)                                       \
    eggs::test_support::check(                                           \
        static_cast<bool>(__VA_ARGS__), __FILE__, __LINE__, #__VA_ARGS__ \
    )
