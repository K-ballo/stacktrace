// Eggs.Stacktrace
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <eggs/stacktrace/detail/noinline.hpp>

// EGGS_STACKTRACE_NO_TAIL_CALLS - suppresses tail-call/sibling-call
// elimination for a function definition.
//
// No portable equivalent exists (e.g. MSVC has none), so this expands to
// nothing on compilers without support.
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

// EGGS_STACKTRACE_PIN_FRAME - best-effort attribute to keep a function's own
// frame in stacktraces captured from within it or from one of its callees.
//
// Apply directly to a function definition:
//
//   EGGS_STACKTRACE_PIN_FRAME void handle_request()
//   {
//       ...
//   }
//
// This suppresses both inlining and tail-call/sibling-call elimination for
// the function it is attached to, which are the two optimisations that would
// otherwise most commonly remove it from a captured stacktrace. It is best
// effort, not a guarantee:
//   - tail-call elimination cannot be suppressed on every compiler (e.g.
//     MSVC); on those, this macro only suppresses inlining, and the frame
//     may still be elided by a tail call.
//   - other transformations (e.g. interprocedural optimisation, LTO) are not
//     accounted for and may still remove the frame.
//
// Suppressing tail-call elimination applies to the whole function body, not
// just a specific call, so prefer using this on boundary/wrapper functions
// rather than on hot paths.
#ifndef EGGS_STACKTRACE_PIN_FRAME
#    define EGGS_STACKTRACE_PIN_FRAME \
        EGGS_STACKTRACE_NOINLINE EGGS_STACKTRACE_NO_TAIL_CALLS
#endif
