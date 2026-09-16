// Eggs.Stacktrace
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#include <eggs/stacktrace.hpp>

#include "support.hpp"
#if __cplusplus >= 202002L
#    include <compare>
#endif
#include <functional>
#include <sstream>

int main()
{
    // -- Default construction --------------------------------------------------

    eggs::stacktrace_entry e;

    EGGS_STACKTRACE_CHECK(e.native_handle() == nullptr);
    EGGS_STACKTRACE_CHECK(!e);
    EGGS_STACKTRACE_CHECK(e.description() == "");
    EGGS_STACKTRACE_CHECK(e.source_file() == "");
    EGGS_STACKTRACE_CHECK(e.source_line() == 0u);

    // -- Equality --------------------------------------------------------------

    eggs::stacktrace_entry e2;

    EGGS_STACKTRACE_CHECK(e == e2);
    EGGS_STACKTRACE_CHECK(!(e != e2));

    // -- Ordering --------------------------------------------------------------

#if __cplusplus >= 202002L
    EGGS_STACKTRACE_CHECK((e <=> e2) == std::strong_ordering::equal);
#endif
    EGGS_STACKTRACE_CHECK(!(e < e2));
    EGGS_STACKTRACE_CHECK(!(e > e2));
    EGGS_STACKTRACE_CHECK(e <= e2);
    EGGS_STACKTRACE_CHECK(e >= e2);

    // -- Hash ------------------------------------------------------------------

    std::hash<eggs::stacktrace_entry> h;
    EGGS_STACKTRACE_CHECK(h(e) == h(e2));

    // -- operator<< --------------------------------------------------------

    std::ostringstream oss;
    oss << e;
    EGGS_STACKTRACE_CHECK(oss.str() == "");

    return eggs::test_support::report();
}
