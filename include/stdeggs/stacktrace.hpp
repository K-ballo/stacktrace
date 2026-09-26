// Eggs.Stacktrace
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#pragma once

// Aliases stdeggs::stacktrace_entry and stdeggs::stacktrace to the standard
// library facilities when available (__cpp_lib_stacktrace), and to the eggs::
// implementation otherwise.
//
// Define EGGS_STACKTRACE_STD_USE_EGGS to always use the eggs:: implementation,
// e.g. so that translation units built against different C++ standards agree
// on what stdeggs::stacktrace names.

#if defined(__has_include)
#    if __has_include(<version>)
#        include <version>
#    endif
#endif

#if defined(__cpp_lib_stacktrace) && !defined(EGGS_STACKTRACE_STD_USE_EGGS)
#    include <stacktrace>
#else
#    include <eggs/stacktrace.hpp>
#endif

namespace stdeggs {

#if defined(__cpp_lib_stacktrace) && !defined(EGGS_STACKTRACE_STD_USE_EGGS)
using ::std::stacktrace;
using ::std::stacktrace_entry;
#else
using ::eggs::stacktrace;
using ::eggs::stacktrace_entry;
#endif

} // namespace stdeggs
