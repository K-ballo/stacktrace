// Eggs.Stacktrace
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#pragma once

// EGGS_STACKTRACE_NOINLINE - suppresses inlining on a function definition.
//
// Not part of the documented public API; subject to change without notice.

#ifndef EGGS_STACKTRACE_NOINLINE
#    if defined(_MSC_VER)
#        define EGGS_STACKTRACE_NOINLINE __declspec(noinline)
#    elif defined(__GNUC__)
#        define EGGS_STACKTRACE_NOINLINE [[gnu::noinline]]
#    else
#        define EGGS_STACKTRACE_NOINLINE
#    endif
#endif
