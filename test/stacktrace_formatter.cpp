// Eggs.Stacktrace
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#include <eggs/stacktrace.hpp>

#ifdef __cpp_lib_format

#    include <format>

#    include "support.hpp"

int main()
{
    // Default-constructed (null) entry formats to empty string.
    eggs::stacktrace_entry e;
    EGGS_STACKTRACE_CHECK(std::format("{}", e).empty());

    // Empty stacktrace formats to empty string.
    eggs::stacktrace st;
    EGGS_STACKTRACE_CHECK(std::format("{}", st).empty());

    return eggs::test_support::report();
}

#else

int main()
{
    return 0;
}

#endif
