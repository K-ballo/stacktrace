// Eggs.Stacktrace
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#include <integration/context.hpp>
#include <integration/plugin.hpp>

namespace integration {
namespace plugin {

namespace {

INTEGRATION_NOINLINE void plugin_hidden_hop(context& ctx, callback cb)
{
    INTEGRATION_CALL(ctx, "plugin_hidden_hop", cb(ctx));
}

} // namespace

INTEGRATION_NOINLINE void plugin_forward(context& ctx, callback cb)
{
    INTEGRATION_CALL(ctx, "plugin_forward", plugin_hidden_hop(ctx, cb));
}

} // namespace plugin
} // namespace integration
