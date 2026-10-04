<!--
Eggs.Stacktrace

Copyright (c) 2026 Agustin Berge

Distributed under the Boost Software License, Version 1.0. (See
accompanying file LICENSE.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
-->

# Reference

## Headers

| Header | Provides |
|--------|----------|
| `<eggs/stacktrace.hpp>` | `eggs::stacktrace`, `eggs::stacktrace_entry`, `to_string`, `operator<<`, `std::hash`, `std::formatter` |

Headers under `eggs/stacktrace/detail/` are implementation details.

---

## `eggs::stacktrace_entry`

A single stack frame, identified by its instruction address.

```cpp
class stacktrace_entry
{
  public:
    using native_handle_type = void*;

    constexpr stacktrace_entry() noexcept = default;

    constexpr native_handle_type native_handle() const noexcept;
    constexpr explicit operator bool() const noexcept;

    std::string description() const;
    std::string source_file() const;
    std::uint_least32_t source_line() const;
};
```

| Member | Description |
|--------|-------------|
| `native_handle()` | The instruction address. `nullptr` for an empty entry. |
| `operator bool` | `false` only if the entry is empty. |
| `description()` | Demangled function name, or empty if unavailable. |
| `source_file()` | Source file path, or empty if unavailable. |
| `source_line()` | Source line, or `0` if unavailable. |

The query functions treat all errors other than memory allocation
failure as "no information available".

**Comparison:** `==` and `!=` are `constexpr` and compare addresses. Two
empty entries are equal. Ordering (`<=>` in C++20, otherwise `<`, `>`,
`<=`, `>=`) is by address.

---

## `eggs::stacktrace`

An immutable sequence of `stacktrace_entry`, innermost frame first.

```cpp
class stacktrace
{
  public:
    using value_type = stacktrace_entry;
    using const_reference = stacktrace_entry const&;
    using reference = const_reference;
    using const_iterator = stacktrace_entry const*;
    using iterator = const_iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using difference_type = std::ptrdiff_t;
    using size_type = std::size_t;

    static stacktrace current() noexcept;
    static stacktrace current(size_type skip) noexcept;
    static stacktrace current(size_type skip, size_type max_depth) noexcept;

    stacktrace() noexcept = default;
    // copy and move construction and assignment

    // begin, end, cbegin, cend, rbegin, rend, crbegin, crend
    bool empty() const noexcept;
    size_type size() const noexcept;
    size_type max_size() const noexcept;

    const_reference operator[](size_type idx) const noexcept;
    const_reference at(size_type idx) const;

    void swap(stacktrace& other) noexcept;
};
```

### Capture

| Function | Description |
|----------|-------------|
| `current()` | Stacktrace of the calling thread, starting at the caller of `current`. Empty if it cannot be obtained. |
| `current(skip)` | As above, omitting the first `skip` frames. |
| `current(skip, max_depth)` | As above, keeping at most `max_depth` frames. |

All overloads are `noexcept`. A `skip` larger than the stack depth
yields an empty stacktrace.

### Element access

| Function | Description |
|----------|-------------|
| `operator[](idx)` | Entry at `idx`. Precondition: `idx < size()`. |
| `at(idx)` | Entry at `idx`. Throws `std::out_of_range` if `idx >= size()`. |

### Comparison

`==` and `!=` compare element-wise. Ordering compares `size()` first,
then lexicographically (`<=>` in C++20, otherwise `<`, `>`, `<=`, `>=`).

---

## Non-member functions

```cpp
void swap(stacktrace& a, stacktrace& b) noexcept;

std::string to_string(stacktrace_entry entry);
std::string to_string(stacktrace const& st);

std::ostream& operator<<(std::ostream& os, stacktrace_entry entry);
std::ostream& operator<<(std::ostream& os, stacktrace const& st);
```

### Output format

`to_string(entry)` is empty for an empty entry. Otherwise:

```
<address>[ in <description>][ at <source_file>[:<source_line>]]
```

The ` in ` part is present when `description()` is non-empty, the
` at ` part when `source_file()` is non-empty, and `:<source_line>`
when additionally `source_line()` is not `0`.

`to_string(st)` is the concatenation of `to_string(entry) + "\n"` for
every entry.

---

## Hashing

`std::hash<eggs::stacktrace_entry>` and `std::hash<eggs::stacktrace>`
are specialized. The stacktrace hash combines the size and all entries.

---

## Formatting

Available when `__cpp_lib_format` is defined.

| Specialization | Format specification |
|----------------|----------------------|
| `std::formatter<eggs::stacktrace_entry>` | Fill, align and width, as for `std::string` |
| `std::formatter<eggs::stacktrace>` | None; produces `to_string(st)` |
