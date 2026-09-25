// Eggs.Stacktrace
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <eggs/stacktrace.hpp>

#include <cstddef>

#include <integration/context.hpp>

namespace integration {
namespace core {

struct traced_context : context
{
    eggs::stacktrace trace;
    std::size_t max_frames = 0; // 0 when unbounded
};

// Signature of the entry point exported by the runtime-loaded module.
using module_forward_fn = void (*)(context&, callback);

// -- Scenarios -----------------------------------------------------------------
// Each one captures a trace into `ctx.trace` and records the anchors it is
// expected to go through.

// Two threads capturing and symbolizing concurrently.
void thread_entry(traced_context& ctx);

// core -> plugin (shared library) -> callback into core.
void chain_entry(traced_context& ctx);

// Captured in an exception constructor, thrown from a template.
void exception_entry(traced_context& ctx);

// Captured in a destructor while unwinding.
void unwind_entry(traced_context& ctx);

// Captured in a comparator called back from the C library.
void qsort_entry(traced_context& ctx);

// Captured in a lambda called through std::function.
void function_entry(traced_context& ctx);

// Captured in a function with a very long demangled name.
void long_name_entry(traced_context& ctx);

// Captured in a branch likely split into a separate cold section.
void cold_entry(traced_context& ctx);

// Captured deep in a recursion, with a capped depth.
void recursion_entry(traced_context& ctx);

// core -> module (loaded at runtime) -> callback into core.
void module_entry(traced_context& ctx, module_forward_fn forward);

} // namespace core
} // namespace integration
