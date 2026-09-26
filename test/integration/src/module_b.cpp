// Eggs.Stacktrace
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

// Loaded after the other module is unloaded, possibly at the same address;
// every name differs so that stale information about the other one shows.

#include <integration/context.hpp>

namespace {

int volatile opaque_sink = 0;

// Shifts the functions below relative to their counterparts in the other
// module, should it have been loaded at the same address.
INTEGRATION_NOINLINE void module_b_padding()
{
    for (int i = 0; i < 16; ++i) opaque_sink = opaque_sink + i;
}

INTEGRATION_NOINLINE void
module_b_hidden_hop(integration::context& ctx, integration::callback cb)
{
    INTEGRATION_CALL(ctx, "module_b_hidden_hop", cb(ctx));
    module_b_padding(); // not a tail call
}

} // namespace

// Looked up by name after the module is loaded at runtime.
extern "C" INTEGRATION_EXPORT INTEGRATION_NOINLINE void
integration_module_b_forward(
    integration::context& ctx, integration::callback cb
)
{
    INTEGRATION_CALL(
        ctx, "integration_module_b_forward", module_b_hidden_hop(ctx, cb)
    );
}
