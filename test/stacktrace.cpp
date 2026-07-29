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

#include "detail/assert.hpp"

int main()
{
    // -- current() returns empty stacktrace with null backend ------------------

    auto st = eggs::stacktrace::current();
    EGGS_TEST_ASSERT(st.empty());
    EGGS_TEST_ASSERT(st.size() == 0u);
    EGGS_TEST_ASSERT(st.begin() == st.end());
    EGGS_TEST_ASSERT(st.cbegin() == st.cend());
    EGGS_TEST_ASSERT(st.rbegin() == st.rend());
    EGGS_TEST_ASSERT(st.crbegin() == st.crend());
    EGGS_TEST_ASSERT(st.crbegin() == st.rbegin());

    // -- Two-parameter overload ------------------------------------------------

    auto st2 = eggs::stacktrace::current(0, 10);
    EGGS_TEST_ASSERT(st2.empty());

    auto st3 = eggs::stacktrace::current(5);
    EGGS_TEST_ASSERT(st3.empty());

    // -- max_size --------------------------------------------------------------

    EGGS_TEST_ASSERT(st.max_size() > 0u);

    // -- Element access --------------------------------------------------------

    bool threw = false;
    try {
        (void)st.at(0);
    } catch (std::out_of_range const&) {
        threw = true;
    }
    EGGS_TEST_ASSERT(threw);

    // -- Copy / move construction ------------------------------------------------

    eggs::stacktrace copied(st);
    EGGS_TEST_ASSERT(copied.empty());
    EGGS_TEST_ASSERT(copied == st);

    eggs::stacktrace moved(std::move(copied));
    EGGS_TEST_ASSERT(moved.empty());
    EGGS_TEST_ASSERT(moved == st);

    eggs::stacktrace copy_assigned;
    copy_assigned = st;
    EGGS_TEST_ASSERT(copy_assigned == st);

    eggs::stacktrace move_assigned;
    move_assigned = std::move(copy_assigned);
    EGGS_TEST_ASSERT(move_assigned == st);

    // -- swap ------------------------------------------------------------------

    eggs::stacktrace a, b;
    a.swap(b);
    EGGS_TEST_ASSERT(a.empty() && b.empty());

    swap(a, b);
    EGGS_TEST_ASSERT(a.empty() && b.empty());

    // -- operator== -----------------------------------------------------------

    EGGS_TEST_ASSERT(a == b);
    EGGS_TEST_ASSERT(!(a != b));

    // -- Ordering ----------------------------------------------------------------

#if __cplusplus >= 202002L
    EGGS_TEST_ASSERT((a <=> b) == std::strong_ordering::equal);
#endif
    EGGS_TEST_ASSERT(!(a < b));
    EGGS_TEST_ASSERT(!(a > b));
    EGGS_TEST_ASSERT(a <= b);
    EGGS_TEST_ASSERT(a >= b);

    // -- to_string -------------------------------------------------------------

    EGGS_TEST_ASSERT(eggs::to_string(st) == "");

    // -- operator<< -----------------------------------------------------------

    std::ostringstream oss;
    oss << st;
    EGGS_TEST_ASSERT(oss.str() == "");

    // -- Hash --------------------------------------------------------------------

    std::hash<eggs::stacktrace> h;
    EGGS_TEST_ASSERT(h(st) == h(a));

    return 0;
}
