// Eggs.Stacktrace
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <algorithm>
#if __cplusplus >= 202002L
#    include <compare>
#endif
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef __cpp_lib_format
#    include <format>
#endif

#include <eggs/stacktrace/detail/nodiscard.hpp>

namespace eggs {

namespace detail {
class capture_helper;
} // namespace detail

// -- [stacktrace.entry] -------------------------------------------------------

class stacktrace_entry
{
  public:
    // [deviation]: native_handle_type is implementation-defined; we use void*.
    using native_handle_type = void*;

    // [stacktrace.entry.cons]
    constexpr stacktrace_entry() noexcept = default;
    constexpr stacktrace_entry(stacktrace_entry const& other) noexcept =
        default;
    /*constexpr(14)*/ stacktrace_entry&
    operator=(stacktrace_entry const& other) noexcept = default;
    ~stacktrace_entry() = default;

    // [stacktrace.entry.obs]
    EGGS_STACKTRACE_NODISCARD constexpr native_handle_type
    native_handle() const noexcept
    {
        return address_;
    }

    // Returns: false only if *this is empty.
    constexpr explicit operator bool() const noexcept
    {
        return address_ != nullptr;
    }

    // [stacktrace.entry.query]
    // Errors other than memory allocation failures are treated as "no
    // information available" and do not cause exceptions to be thrown.
    EGGS_STACKTRACE_NODISCARD std::string description() const;
    EGGS_STACKTRACE_NODISCARD std::string source_file() const;
    EGGS_STACKTRACE_NODISCARD std::uint_least32_t source_line() const;

    // [stacktrace.entry.cmp]
    // Returns: true iff x and y represent the same entry or both are empty.
    // [deviation]: operator<=> cannot be constexpr because ordering uses
    //   reinterpret_cast; P2448R2 (C++23) relaxes this but C++20 does not.
    friend constexpr bool
    operator==(stacktrace_entry const& x, stacktrace_entry const& y) noexcept
    {
        return x.address_ == y.address_;
    }

    friend constexpr bool
    operator!=(stacktrace_entry const& x, stacktrace_entry const& y) noexcept
    {
        return !(x == y);
    }

#if __cplusplus >= 202002L
    friend std::strong_ordering
    operator<=>(stacktrace_entry const& x, stacktrace_entry const& y) noexcept
    {
        auto const xv = reinterpret_cast<std::uintptr_t>(x.address_);
        auto const yv = reinterpret_cast<std::uintptr_t>(y.address_);
        return xv <=> yv;
    }
#else
    friend bool
    operator<(stacktrace_entry const& x, stacktrace_entry const& y) noexcept
    {
        auto const xv = reinterpret_cast<std::uintptr_t>(x.address_);
        auto const yv = reinterpret_cast<std::uintptr_t>(y.address_);
        return xv < yv;
    }

    friend bool
    operator>(stacktrace_entry const& x, stacktrace_entry const& y) noexcept
    {
        return y < x;
    }

    friend bool
    operator<=(stacktrace_entry const& x, stacktrace_entry const& y) noexcept
    {
        return !(y < x);
    }

    friend bool
    operator>=(stacktrace_entry const& x, stacktrace_entry const& y) noexcept
    {
        return !(x < y);
    }
#endif

  private:
    constexpr explicit stacktrace_entry(void* addr) noexcept
        : address_(addr)
    {
    }

    void* address_ = nullptr;

    friend class detail::capture_helper;
};

// -- [stacktrace.basic] -------------------------------------------------------

// [deviation]: upstream std::basic_stacktrace<Allocator> is a template;
//   eggs::stacktrace hardcodes std::allocator<stacktrace_entry> instead, so
//   there is no allocator-parameterized alias and no allocator support.
class stacktrace
{
  public:
    using value_type = stacktrace_entry;
    using const_reference = stacktrace_entry const&;
    // [deviation]: standard has reference = value_type& (mutable); we use
    //   const_reference because no mutable element access is provided.
    using reference = const_reference;
    using const_iterator = stacktrace_entry const*;
    using iterator = const_iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using difference_type = std::ptrdiff_t;
    using size_type = std::size_t;

    // [stacktrace.basic.cons]

    // Returns an object with the stacktrace of the current thread of execution,
    // or an empty object if the stacktrace cannot be obtained.
    // skip: number of frames to skip from the top of the captured stacktrace.
    // max_depth: maximum number of frames to capture.
    // [deviation]: [[nodiscard]] is a non-standard extension.
    EGGS_STACKTRACE_NODISCARD static stacktrace current() noexcept;

    EGGS_STACKTRACE_NODISCARD static stacktrace
    current(size_type skip) noexcept;

    EGGS_STACKTRACE_NODISCARD static stacktrace
    current(size_type skip, size_type max_depth) noexcept;

    stacktrace() noexcept = default;

    stacktrace(stacktrace const&) = default;
    stacktrace(stacktrace&&) noexcept = default;

    stacktrace& operator=(stacktrace const&) = default;
    stacktrace& operator=(stacktrace&&) noexcept = default;

    ~stacktrace() = default;

    // [stacktrace.basic.obs]

    EGGS_STACKTRACE_NODISCARD const_iterator begin() const noexcept
    {
        return frames_.data();
    }

    EGGS_STACKTRACE_NODISCARD const_iterator end() const noexcept
    {
        return frames_.data() + frames_.size();
    }

    EGGS_STACKTRACE_NODISCARD const_iterator cbegin() const noexcept
    {
        return begin();
    }

    EGGS_STACKTRACE_NODISCARD const_iterator cend() const noexcept
    {
        return end();
    }

    EGGS_STACKTRACE_NODISCARD const_reverse_iterator rbegin() const noexcept
    {
        return const_reverse_iterator{end()};
    }

    EGGS_STACKTRACE_NODISCARD const_reverse_iterator rend() const noexcept
    {
        return const_reverse_iterator{begin()};
    }

    EGGS_STACKTRACE_NODISCARD const_reverse_iterator crbegin() const noexcept
    {
        return rbegin();
    }

    EGGS_STACKTRACE_NODISCARD const_reverse_iterator crend() const noexcept
    {
        return rend();
    }

    EGGS_STACKTRACE_NODISCARD bool empty() const noexcept
    {
        return frames_.empty();
    }

    EGGS_STACKTRACE_NODISCARD size_type size() const noexcept
    {
        return frames_.size();
    }

    EGGS_STACKTRACE_NODISCARD size_type max_size() const noexcept
    {
        return frames_.max_size();
    }

    // [stacktrace.basic.elem]

    // Precondition: idx < size()
    const_reference operator[](size_type idx) const noexcept
    {
        return frames_[idx];
    }

    EGGS_STACKTRACE_NODISCARD const_reference at(size_type idx) const
    {
        if (idx >= frames_.size())
            throw std::out_of_range{"eggs::stacktrace::at"};
        return frames_[idx];
    }

    // [stacktrace.basic.cmp]

    // Returns: std::equal(x.begin(), x.end(), y.begin(), y.end())
    friend bool operator==(stacktrace const& x, stacktrace const& y) noexcept
    {
        return x.size() == y.size() &&
               std::equal(x.begin(), x.end(), y.begin());
    }

    friend bool operator!=(stacktrace const& x, stacktrace const& y) noexcept
    {
        return !(x == y);
    }

    // Returns: x.size() <=> y.size() if x.size() != y.size();
    //          lexicographical_compare_three_way(...) otherwise.
#if __cplusplus >= 202002L
    friend std::strong_ordering
    operator<=>(stacktrace const& x, stacktrace const& y) noexcept
    {
        if (x.size() != y.size()) return x.size() <=> y.size();
        return std::lexicographical_compare_three_way(
            x.begin(), x.end(), y.begin(), y.end(), std::compare_three_way{}
        );
    }
#else
    friend bool operator<(stacktrace const& x, stacktrace const& y) noexcept
    {
        if (x.size() != y.size()) return x.size() < y.size();
        return std::lexicographical_compare(
            x.begin(), x.end(), y.begin(), y.end()
        );
    }

    friend bool operator>(stacktrace const& x, stacktrace const& y) noexcept
    {
        return y < x;
    }

    friend bool operator<=(stacktrace const& x, stacktrace const& y) noexcept
    {
        return !(y < x);
    }

    friend bool operator>=(stacktrace const& x, stacktrace const& y) noexcept
    {
        return !(x < y);
    }
#endif

    // [stacktrace.basic.mod]

    void swap(stacktrace& other) noexcept { frames_.swap(other.frames_); }

  private:
    std::vector<stacktrace_entry> frames_;
};

// [deviation]: no std::pmr::stacktrace alias.

// -- [stacktrace.basic.nonmem] -------------------------------------------------

inline void swap(stacktrace& a, stacktrace& b) noexcept(noexcept(a.swap(b)))
{
    a.swap(b);
}

std::string to_string(stacktrace_entry entry);
std::string to_string(stacktrace const& st);

// Stream insertion operators use std::ostream (not basic_ostream<CharT,Traits>).
inline std::ostream& operator<<(std::ostream& os, stacktrace_entry entry)
{
    return os << to_string(entry);
}

inline std::ostream& operator<<(std::ostream& os, stacktrace const& st)
{
    return os << to_string(st);
}

} // namespace eggs

// -- [stacktrace.hash] ---------------------------------------------------------

template <>
struct std::hash<eggs::stacktrace_entry>
{
    std::size_t operator()(eggs::stacktrace_entry e) const noexcept
    {
        return std::hash<void*>{}(e.native_handle());
    }
};

template <>
struct std::hash<eggs::stacktrace>
{
    std::size_t operator()(eggs::stacktrace const& st) const noexcept
    {
        std::size_t seed = st.size();
        for (auto const& e : st)
            seed ^= std::hash<eggs::stacktrace_entry>{}(e) + 0x9e3779b9U +
                    (seed << 6) + (seed >> 2);
        return seed;
    }
};

// -- [stacktrace.format] -------------------------------------------------------

#ifdef __cpp_lib_format

// stacktrace-entry-format-spec supports fill-and-align and width.
// Delegating to formatter<string> handles both correctly.
template <>
struct std::formatter<eggs::stacktrace_entry, char>
{
    std::formatter<std::string, char> underlying_;

    constexpr auto parse(std::format_parse_context& ctx)
    {
        return underlying_.parse(ctx);
    }

    auto format(eggs::stacktrace_entry e, std::format_context& ctx) const
    {
        return underlying_.format(eggs::to_string(e), ctx);
    }
};

// No format-spec is supported for eggs::stacktrace.
template <>
struct std::formatter<eggs::stacktrace, char>
{
    static constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    static auto format(eggs::stacktrace const& st, std::format_context& ctx)
    {
        return std::format_to(ctx.out(), "{}", eggs::to_string(st));
    }
};

#endif
