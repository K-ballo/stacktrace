// Eggs.Stacktrace
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <eggs/stacktrace/detail/no_tail_calls.hpp>
#include <eggs/stacktrace/detail/noinline.hpp>

// EGGS_STACKTRACE_PIN_FRAME - best-effort attribute to keep a function's own
// frame in stacktraces captured from within it or from one of its callees.
//
// Apply directly to a function definition:
//
//   EGGS_STACKTRACE_PIN_FRAME void handle_request()
//   {
//       ...
//   }
#ifndef EGGS_STACKTRACE_PIN_FRAME
#    define EGGS_STACKTRACE_PIN_FRAME \
        EGGS_STACKTRACE_NOINLINE EGGS_STACKTRACE_NO_TAIL_CALLS
#endif
