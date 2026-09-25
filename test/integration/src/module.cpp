// Eggs.Stacktrace
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#include <integration/context.hpp>

namespace {

INTEGRATION_NOINLINE void
module_hidden_hop(integration::context& ctx, integration::callback cb)
{
    INTEGRATION_CALL(ctx, "module_hidden_hop", cb(ctx));
}

} // namespace

// Looked up by name after the module is loaded at runtime.
extern "C" INTEGRATION_EXPORT INTEGRATION_NOINLINE void
integration_module_forward(integration::context& ctx, integration::callback cb)
{
    INTEGRATION_CALL(
        ctx, "integration_module_forward", module_hidden_hop(ctx, cb)
    );
}
