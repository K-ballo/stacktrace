// Eggs.Stacktrace
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#include <eggs/stacktrace.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "detail/backend.hpp"

namespace eggs {
namespace detail {

void capture(
    std::vector<stacktrace_entry>& /*unused*/, std::size_t /*unused*/,
    std::size_t /*unused*/
) noexcept
{
}

std::string symbolize_description(void* /*unused*/)
{
    return {};
}

std::string symbolize_source_file(void* /*unused*/)
{
    return {};
}

std::uint_least32_t symbolize_source_line(void* /*unused*/)
{
    return 0;
}

} // namespace detail
} // namespace eggs
