// Eggs.Stacktrace
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#include <eggs/stacktrace.hpp>

#include <cstdlib>
#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

#include <integration/context.hpp>
#include <integration/core.hpp>
#include <integration/plugin.hpp>

#if defined(_MSC_VER)
#    define INTEGRATION_COLD
#else
#    define INTEGRATION_COLD __attribute__((cold))
#endif

// Records the enclosing function `name` as an anchor at this line, and as
// the function expected at frame 0, then captures the current stacktrace.
#define INTEGRATION_CAPTURE(ctx, name)                       \
    ((ctx).note(name, __FILE__, __LINE__), (ctx).top = name, \
     ::eggs::stacktrace::current())

#define INTEGRATION_CAPTURE_N(ctx, name, skip, max_depth)    \
    ((ctx).note(name, __FILE__, __LINE__), (ctx).top = name, \
     ::eggs::stacktrace::current(skip, max_depth))

namespace integration {
namespace core {

namespace {

// Keeps the optimizer from folding away branches and side-effect free calls.
bool volatile opaque_true = true;
int volatile opaque_sink = 0;

traced_context& traced(context& ctx)
{
    return static_cast<traced_context&>(ctx);
}

} // namespace

// -- thread --------------------------------------------------------------------

namespace {

INTEGRATION_NOINLINE void thread_body(traced_context& ctx)
{
    ctx.trace = INTEGRATION_CAPTURE(ctx, "thread_body");
    // First symbolization, likely racing the other thread.
    opaque_sink = static_cast<int>(eggs::to_string(ctx.trace).size());
}

} // namespace

void thread_entry(traced_context& ctx)
{
    traced_context other;
    std::thread first(&thread_body, std::ref(ctx));
    std::thread second(&thread_body, std::ref(other));
    first.join();
    second.join();
}

// -- chain ---------------------------------------------------------------------

namespace {

INTEGRATION_NOINLINE void chain_callback(context& ctx)
{
    traced(ctx).trace = INTEGRATION_CAPTURE(ctx, "chain_callback");
}

} // namespace

INTEGRATION_NOINLINE void chain_entry(traced_context& ctx)
{
    INTEGRATION_CALL(
        ctx, "chain_entry", plugin::plugin_forward(ctx, &chain_callback)
    );
}

// -- exception -----------------------------------------------------------------

namespace {

struct traced_error : std::runtime_error
{
    eggs::stacktrace trace;

    // skip=1: the trace starts at the throw site, not in this constructor.
    // Not inlined, or skip=1 would skip the throw site instead.
    INTEGRATION_NOINLINE traced_error()
        : std::runtime_error("traced_error"),
          trace(eggs::stacktrace::current(1))
    {
    }
};

template <typename T>
[[noreturn]] INTEGRATION_NOINLINE void throw_traced(traced_context& ctx)
{
    ctx.top = "throw_traced";
    INTEGRATION_CALL(ctx, "throw_traced", throw traced_error());
}

} // namespace

INTEGRATION_NOINLINE void exception_entry(traced_context& ctx)
{
    try {
        INTEGRATION_CALL(
            ctx, "exception_entry", throw_traced<std::vector<int>>(ctx)
        );
    } catch (traced_error const& e) {
        ctx.trace = e.trace;
    }
}

// -- unwind --------------------------------------------------------------------

namespace {

struct unwind_probe
{
    traced_context& ctx;

    explicit unwind_probe(traced_context& target)
        : ctx(target)
    {
    }

    unwind_probe(unwind_probe const&) = delete;
    unwind_probe& operator=(unwind_probe const&) = delete;

    INTEGRATION_NOINLINE ~unwind_probe()
    {
        ctx.trace = INTEGRATION_CAPTURE(ctx, "unwind_probe");
    }
};

INTEGRATION_NOINLINE void unwind_thrower(traced_context& ctx)
{
    unwind_probe const probe{ctx};
    // The destructor runs from a landing pad; its line is unpredictable.
    ctx.note("unwind_thrower", __FILE__, 0);
    if (opaque_true) throw std::runtime_error("unwind");
}

} // namespace

INTEGRATION_NOINLINE void unwind_entry(traced_context& ctx)
{
    try {
        INTEGRATION_CALL(ctx, "unwind_entry", unwind_thrower(ctx));
    } catch (std::runtime_error const&) {
        opaque_sink = 1;
    }
}

// -- qsort ---------------------------------------------------------------------

namespace {

traced_context* qsort_ctx = nullptr;

int qsort_compare(void const* lhs, void const* rhs)
{
    if (qsort_ctx != nullptr) {
        qsort_ctx->trace = INTEGRATION_CAPTURE(*qsort_ctx, "qsort_compare");
        qsort_ctx = nullptr;
    }
    int const l = *static_cast<int const*>(lhs);
    int const r = *static_cast<int const*>(rhs);
    return (l > r) - (l < r);
}

} // namespace

INTEGRATION_NOINLINE void qsort_entry(traced_context& ctx)
{
    int values[] = {3, 1, 2, opaque_sink};
    qsort_ctx = &ctx;
    INTEGRATION_CALL(
        ctx, "qsort_entry", std::qsort(values, 4, sizeof(int), &qsort_compare)
    );
    opaque_sink = values[0];
}

// -- function ------------------------------------------------------------------

namespace {

INTEGRATION_NOINLINE void
invoke_function(traced_context& ctx, std::function<void()> const& fn)
{
    INTEGRATION_CALL(ctx, "invoke_function", fn());
}

} // namespace

INTEGRATION_NOINLINE void function_entry(traced_context& ctx)
{
    // The lambda's name is derived from the enclosing function's.
    std::function<void()> const fn = [&ctx] {
        ctx.trace = INTEGRATION_CAPTURE(ctx, "function_entry");
        // May be inlined into std::function's invoker, whose name need not
        // mention function_entry.
        ctx.top = nullptr;
    };
    INTEGRATION_CALL(ctx, "function_entry", invoke_function(ctx, fn));
}

// -- long_name -----------------------------------------------------------------

namespace {

using long_type = std::map<
    std::string,
    std::vector<std::pair<
        std::string,
        std::tuple<int, double, std::string, std::map<int, std::string>>>>>;

template <typename T>
struct long_name_holder
{
    template <typename U>
    INTEGRATION_NOINLINE static void long_name_leaf(traced_context& ctx)
    {
        ctx.trace = INTEGRATION_CAPTURE(ctx, "long_name_leaf");
    }
};

} // namespace

INTEGRATION_NOINLINE void long_name_entry(traced_context& ctx)
{
    INTEGRATION_CALL(
        ctx, "long_name_entry",
        (long_name_holder<long_type>::long_name_leaf<
            std::map<long_type, long_type>>(ctx))
    );
}

// -- cold ----------------------------------------------------------------------

namespace {

// Calling a cold function marks the calling block cold, which GCC moves to
// a separate `.cold` part of the function under optimization.
INTEGRATION_COLD INTEGRATION_NOINLINE void cold_marker()
{
    opaque_sink = 2;
}

INTEGRATION_NOINLINE void cold_split(traced_context& ctx, bool rare)
{
    if (rare) {
        cold_marker();
        ctx.trace = INTEGRATION_CAPTURE(ctx, "cold_split");
        return;
    }
    opaque_sink = 3;
}

} // namespace

INTEGRATION_NOINLINE void cold_entry(traced_context& ctx)
{
    INTEGRATION_CALL(ctx, "cold_entry", cold_split(ctx, opaque_true));
}

// -- recursion -----------------------------------------------------------------

namespace {

INTEGRATION_NOINLINE void recurse(traced_context& ctx, int depth)
{
    if (depth == 0) {
        ctx.trace = INTEGRATION_CAPTURE_N(ctx, "recurse", 0, ctx.max_frames);
        return;
    }
    recurse(ctx, depth - 1);
    opaque_sink = depth; // not a tail call
}

} // namespace

INTEGRATION_NOINLINE void recursion_entry(traced_context& ctx)
{
    // Not an anchor: this frame is beyond the capped depth.
    ctx.max_frames = 16;
    recurse(ctx, 64);
}

// -- module --------------------------------------------------------------------

namespace {

INTEGRATION_NOINLINE void module_callback(context& ctx)
{
    traced(ctx).trace = INTEGRATION_CAPTURE(ctx, "module_callback");
}

} // namespace

INTEGRATION_NOINLINE void
module_entry(traced_context& ctx, module_forward_fn forward)
{
    INTEGRATION_CALL(ctx, "module_entry", forward(ctx, &module_callback));
}

} // namespace core
} // namespace integration
