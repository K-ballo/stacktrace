// Eggs.Stacktrace
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

// Demonstrates:
//   - an exception base class that embeds a stacktrace on construction
//   - the trace is captured at the throw site, not in the catch handler
//   - the same call chain as basic.cpp so the output is easy to compare

#include <eggs/stacktrace.hpp>

#include <iostream>
#include <stdexcept>
#include <string>

#ifdef __cpp_lib_format
#    include <format>
#endif

// -- Exception type ------------------------------------------------------------

struct traced_error : std::runtime_error
{
    eggs::stacktrace trace;

    explicit traced_error(std::string const& msg)
        : std::runtime_error(msg)
          // skip=1: skip the constructor itself so the trace starts at the
          // throw site, not inside traced_error::traced_error.
          ,
          trace(eggs::stacktrace::current(1))
    {
    }

    traced_error(traced_error const&) = default;
    traced_error& operator=(traced_error const&) = default;
    ~traced_error() override;
};

traced_error::~traced_error() = default;

// -- Same call chain as basic.cpp ----------------------------------------------

namespace example {

namespace {

[[noreturn]] EGGS_STACKTRACE_PIN_FRAME void innermost()
{
    throw traced_error("something went wrong");
}

// Not pinned: may be inlined into middle() under optimisation.
// Debug: visible as a distinct (anonymous namespace)::adapt frame.
// Release: typically absent - merged into middle().
[[noreturn]] void adapt()
{
    innermost();
}

} // namespace

[[noreturn]] void middle();
[[noreturn]] void dispatch();

[[noreturn]] EGGS_STACKTRACE_PIN_FRAME void middle()
{
    adapt();
}

// Not pinned: may be inlined into outer<42>() under optimisation.
// Debug: visible as example::dispatch.
// Release: typically absent - merged into outer<42>().
[[noreturn]] void dispatch()
{
    middle();
}

template <int N>
EGGS_STACKTRACE_PIN_FRAME void outer()
{
    dispatch();
}

} // namespace example

// -- main ----------------------------------------------------------------------

int main()
{
    try {
        example::outer<42>();
    } catch (traced_error const& e) {
        std::cout << "error: " << e.what() << "\n\n";
        std::cout << "=== captured at throw site ===\n";
#ifdef __cpp_lib_format
        std::cout << std::format("{}", e.trace);
#else
        std::cout << e.trace;
#endif
    }
}
