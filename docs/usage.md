<!--
Eggs.Stacktrace

Copyright (c) 2026 Agustin Berge

Distributed under the Boost Software License, Version 1.0. (See
accompanying file LICENSE.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
-->

# Usage

## Capturing a stacktrace

```cpp
#include <eggs/stacktrace.hpp>
#include <iostream>

void foo()
{
    eggs::stacktrace st = eggs::stacktrace::current();
    std::cout << st;
}
```

`stacktrace::current()` captures the calling thread's stack, starting
at the function that called it. Stream insertion and `to_string` print
one entry per line.

---

## Skipping frames and limiting depth

```cpp
auto a = eggs::stacktrace::current();        // starts at the caller
auto b = eggs::stacktrace::current(1);       // skip 1 frame
auto c = eggs::stacktrace::current(1, 8);    // skip 1, keep at most 8
```

---

## Inspecting entries

A `stacktrace` is a random-access sequence of `stacktrace_entry`.

```cpp
for (eggs::stacktrace_entry const& e : st) {
    std::cout << e.native_handle() << "\n"
              << "  " << e.description() << "\n"
              << "  " << e.source_file() << ":" << e.source_line() << "\n";
}
```

`description()` is the (demangled) function name; `source_file()` and
`source_line()` give the source location. Any of them may be empty (or
`0`) depending on the backend and on whether debug information is
available (`-g`, `/Zi`). See [Design](design.md) for what each backend
provides.

---

## Formatting

With `<format>` available, both types are usable with `std::format`:

```cpp
std::cout << std::format("{}", st);         // whole trace
std::cout << std::format("{:>40}", st[0]);  // one entry, with width/align
```

`stacktrace_entry` accepts a fill-and-align and width; `stacktrace`
accepts no format specification.

---

## Capturing in an exception

Capture the trace where the exception is created, not where it is
caught. Skip the constructor's own frame so the trace starts at the
throw site:

```cpp
struct traced_error : std::runtime_error
{
    eggs::stacktrace trace;

    explicit traced_error(std::string const& msg)
        : std::runtime_error(msg)
        , trace(eggs::stacktrace::current(1))
    {
    }
};
```

See `example/exception.cpp` for a complete program.

---

## Hashing and comparison

Entries and stacktraces are equality comparable and totally ordered
(`<=>` in C++20, relational operators before), and `std::hash` is
specialized for both. This makes them usable as keys, for example to
deduplicate error reports by call stack:

```cpp
std::unordered_set<eggs::stacktrace> seen;
if (seen.insert(eggs::stacktrace::current()).second)
    report_new_site();
```
