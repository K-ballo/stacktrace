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

void capture(std::vector<stacktrace_entry>&, std::size_t, std::size_t) noexcept
{
}

std::string symbolize_description(void*)
{
    return {};
}

std::string symbolize_source_file(void*)
{
    return {};
}

std::uint_least32_t symbolize_source_line(void*)
{
    return 0;
}

} // namespace detail
} // namespace eggs
