// Eggs.Stacktrace
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#include <eggs/stacktrace.hpp>

#if __cplusplus >= 202002L
#    include <version>
#endif

#ifdef __cpp_lib_format

#    include <format>

#    include "detail/assert.hpp"

int main()
{
    // Default-constructed (null) entry formats to empty string.
    eggs::stacktrace_entry e;
    EGGS_TEST_ASSERT(std::format("{}", e) == "");

    // Empty stacktrace formats to empty string.
    eggs::stacktrace st;
    EGGS_TEST_ASSERT(std::format("{}", st) == "");

    return 0;
}

#else

int main()
{
    return 0;
}

#endif
