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
#include <cstdint>
#include <string>
#include <vector>

namespace eggs {
namespace detail {

// Factory - constructs stacktrace_entry from a raw address.
// Declared a friend of stacktrace_entry in the public header.
class capture_helper
{
  public:
    static stacktrace_entry make(void* addr) noexcept
    {
        return stacktrace_entry{addr};
    }
};

// -- Backend contract ----------------------------------------------------------

// Fills `frames` with captured entries. Never throws. `skip` already accounts
// for frames above the immediate caller; `max_depth` caps the result size.
void capture(
    std::vector<stacktrace_entry>& frames, std::size_t skip,
    std::size_t max_depth
) noexcept;

// Resolve an address to human-readable information. May return empty / 0.
std::string symbolize_description(void* address);
std::string symbolize_source_file(void* address);
std::uint_least32_t symbolize_source_line(void* address);

} // namespace detail
} // namespace eggs
