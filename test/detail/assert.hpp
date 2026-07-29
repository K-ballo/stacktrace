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

namespace eggs {
namespace test {

[[noreturn]] inline void
assert_failed(char const* file, int line, char const* expr)
{
    std::fprintf(stderr, "%s:%d: assertion failed: %s\n", file, line, expr);
    std::abort();
}

} // namespace test
} // namespace eggs

#define EGGS_TEST_ASSERT(...)       \
    (static_cast<bool>(__VA_ARGS__) \
         ? void(0)                  \
         : eggs::test::assert_failed(__FILE__, __LINE__, #__VA_ARGS__))
