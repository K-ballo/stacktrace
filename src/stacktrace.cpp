// Eggs.Stacktrace
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#include <eggs/stacktrace.hpp>
#include <eggs/stacktrace/pin_frame.hpp>

#include <cstdint>
#include <limits>
#include <sstream>
#include <string>

#include "detail/backend.hpp"

namespace eggs {

// -- stacktrace::current --------------------------------------------------------

EGGS_STACKTRACE_PIN_FRAME
stacktrace stacktrace::current() noexcept
{
    stacktrace st;
    detail::capture(st.frames_, 1, std::numeric_limits<size_type>::max());
    return st;
}

EGGS_STACKTRACE_PIN_FRAME
stacktrace stacktrace::current(size_type skip) noexcept
{
    if (skip == std::numeric_limits<size_type>::max()) return {};

    stacktrace st;
    detail::capture(
        st.frames_, skip + 1, std::numeric_limits<size_type>::max()
    );
    return st;
}

EGGS_STACKTRACE_PIN_FRAME
stacktrace stacktrace::current(size_type skip, size_type max_depth) noexcept
{
    if (skip == std::numeric_limits<size_type>::max()) return {};

    stacktrace st;
    detail::capture(st.frames_, skip + 1, max_depth);
    return st;
}

// -- stacktrace_entry accessors ------------------------------------------------

std::string stacktrace_entry::description() const
{
    return detail::symbolize_description(address_);
}

std::string stacktrace_entry::source_file() const
{
    return detail::symbolize_source_file(address_);
}

std::uint_least32_t stacktrace_entry::source_line() const
{
    return detail::symbolize_source_line(address_);
}

// -- to_string(stacktrace_entry) -----------------------------------------------

std::string to_string(stacktrace_entry entry)
{
    if (!entry) return {};

    std::ostringstream oss;
    oss << entry.native_handle();
    std::string result = oss.str();

    std::string const desc = entry.description();
    if (!desc.empty()) {
        result += " in ";
        result += desc;
    }

    std::string const file = entry.source_file();
    if (!file.empty()) {
        result += " at ";
        result += file;
        std::uint_least32_t const line = entry.source_line();
        if (line != 0) {
            result += ':';
            result += std::to_string(line);
        }
    }

    return result;
}

// -- to_string(stacktrace) -------------------------------------------------------

std::string to_string(stacktrace const& st)
{
    std::string result;
    for (auto const& e : st) {
        result += eggs::to_string(e);
        result += '\n';
    }
    return result;
}

} // namespace eggs
