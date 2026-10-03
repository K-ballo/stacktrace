// Eggs.Stacktrace
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#pragma once

// EGGS_STACKTRACE_NO_TAIL_CALLS - suppresses tail-call/sibling-call
// elimination for a function definition.
//
// Not part of the documented public API; subject to change without notice.

#ifndef EGGS_STACKTRACE_NO_TAIL_CALLS
#    if defined(__clang__)
#        define EGGS_STACKTRACE_NO_TAIL_CALLS \
            __attribute__((disable_tail_calls))
#    elif defined(__GNUC__)
#        define EGGS_STACKTRACE_NO_TAIL_CALLS \
            __attribute__((optimize("no-optimize-sibling-calls")))
#    else
#        define EGGS_STACKTRACE_NO_TAIL_CALLS
#    endif
#endif
