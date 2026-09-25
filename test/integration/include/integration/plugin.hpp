// Eggs.Stacktrace
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <integration/context.hpp>

#if defined(integration_plugin_EXPORTS)
#    define INTEGRATION_PLUGIN_API INTEGRATION_EXPORT
#else
#    define INTEGRATION_PLUGIN_API INTEGRATION_IMPORT
#endif

namespace integration {
namespace plugin {

// Forwards to `cb` through a hidden (non-exported) function.
INTEGRATION_PLUGIN_API void plugin_forward(context& ctx, callback cb);

} // namespace plugin
} // namespace integration
