// Eggs.Stacktrace
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#include <eggs/stacktrace.hpp>

#if __cplusplus >= 202002L
#    include <compare>
#endif
#include <functional>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "support.hpp"

int main()
{
    // -- current() returns empty stacktrace with null backend ------------------

    auto st = eggs::stacktrace::current();
    EGGS_STACKTRACE_CHECK(st.empty());
    EGGS_STACKTRACE_CHECK(st.size() == 0u);
    EGGS_STACKTRACE_CHECK(st.begin() == st.end());
    EGGS_STACKTRACE_CHECK(st.cbegin() == st.cend());
    EGGS_STACKTRACE_CHECK(st.rbegin() == st.rend());
    EGGS_STACKTRACE_CHECK(st.crbegin() == st.crend());
    EGGS_STACKTRACE_CHECK(st.crbegin() == st.rbegin());

    // -- Two-parameter overload ------------------------------------------------

    auto st2 = eggs::stacktrace::current(0, 10);
    EGGS_STACKTRACE_CHECK(st2.empty());

    auto st3 = eggs::stacktrace::current(5);
    EGGS_STACKTRACE_CHECK(st3.empty());

    // -- max_size --------------------------------------------------------------

    EGGS_STACKTRACE_CHECK(st.max_size() > 0u);

    // -- Element access --------------------------------------------------------

    bool threw = false;
    try {
        (void)st.at(0);
    } catch (std::out_of_range const&) {
        threw = true;
    }
    EGGS_STACKTRACE_CHECK(threw);

    // -- Copy / move construction ------------------------------------------------

    eggs::stacktrace copied(st);
    EGGS_STACKTRACE_CHECK(copied.empty());
    EGGS_STACKTRACE_CHECK(copied == st);

    eggs::stacktrace moved(std::move(copied));
    EGGS_STACKTRACE_CHECK(moved.empty());
    EGGS_STACKTRACE_CHECK(moved == st);

    eggs::stacktrace copy_assigned;
    copy_assigned = st;
    EGGS_STACKTRACE_CHECK(copy_assigned == st);

    eggs::stacktrace move_assigned;
    move_assigned = std::move(copy_assigned);
    EGGS_STACKTRACE_CHECK(move_assigned == st);

    // -- swap ------------------------------------------------------------------

    eggs::stacktrace a, b;
    a.swap(b);
    EGGS_STACKTRACE_CHECK(a.empty() && b.empty());

    swap(a, b);
    EGGS_STACKTRACE_CHECK(a.empty() && b.empty());

    // -- operator== -----------------------------------------------------------

    EGGS_STACKTRACE_CHECK(a == b);
    EGGS_STACKTRACE_CHECK(!(a != b));

    // -- Ordering ----------------------------------------------------------------

#if __cplusplus >= 202002L
    EGGS_STACKTRACE_CHECK((a <=> b) == std::strong_ordering::equal);
#endif
    EGGS_STACKTRACE_CHECK(!(a < b));
    EGGS_STACKTRACE_CHECK(!(a > b));
    EGGS_STACKTRACE_CHECK(a <= b);
    EGGS_STACKTRACE_CHECK(a >= b);

    // -- to_string -------------------------------------------------------------

    EGGS_STACKTRACE_CHECK(eggs::to_string(st) == "");

    // -- operator<< -----------------------------------------------------------

    std::ostringstream oss;
    oss << st;
    EGGS_STACKTRACE_CHECK(oss.str() == "");

    // -- Hash --------------------------------------------------------------------

    std::hash<eggs::stacktrace> h;
    EGGS_STACKTRACE_CHECK(h(st) == h(a));

    return eggs::test_support::report();
}
