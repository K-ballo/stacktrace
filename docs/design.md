<!--
Eggs.Stacktrace

Copyright (c) 2026 Agustin Berge

Distributed under the Boost Software License, Version 1.0. (See
accompanying file LICENSE.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
-->

# Design

## Mirrors `std::stacktrace`

The public interface follows the C++23 `<stacktrace>` header section by
section: `stacktrace_entry` ([stacktrace.entry]), `stacktrace`
([stacktrace.basic]), the non-member functions, hashing and formatting.
Code written against `eggs::stacktrace` should port to `std::stacktrace`
by changing the namespace. The deviations are deliberate and small:

- **No allocator parameter.** `eggs::stacktrace` is a plain class, not
  an alias of a `basic_stacktrace<Allocator>` template, and there is no
  `pmr` alias.
- **`reference` is `const_reference`.** No mutable element access is
  provided, so iterators are `stacktrace_entry const*`.
- **`native_handle_type` is `void*`.** It is the instruction address.
- **`std::ostream` only.** Stream insertion is not templated on the
  character type.
- **`current()` is `[[nodiscard]]`** (where the compiler supports it).
- **C++20 comparison is not `constexpr`.** `operator<=>` on entries
  orders by address, which needs `reinterpret_cast`.

## Backends

Capture and symbolization are the only platform-specific parts. They
sit behind a small contract (`capture`, plus one function each for
description, source file and source line) that every backend
implements. Everything else -- containers, comparison, hashing,
formatting -- lives in a common object library shared by all backends.

| Backend | Capture | Symbolization |
|---------|---------|---------------|
| `Null` | none (always empty) | none |
| `Execinfo` | `backtrace()` | `dladdr()` + `__cxa_demangle()`; no source file or line |
| `Libbacktrace` | libbacktrace | function, file and line from debug info |
| `Win32` | `CaptureStackBackTrace` | DbgHelp; function, file and line from the PDB |

Each backend is a separate static library, so several can be built in
the same project (for example to compare them in tests). The
`Eggs::Stacktrace` target aliases one of them: by default the first
available of Libbacktrace, Execinfo and Win32, falling back to Null.

The `Null` backend exists so that code using the library always
builds and links. It never fails and never captures anything, which
makes it a safe choice where no real backend is available.

## Failure is not an error

`stacktrace::current` is `noexcept`. If a trace cannot be obtained the
result is an empty `stacktrace`. Likewise, `description()`,
`source_file()` and `source_line()` return an empty string or `0` when
the backend has no information; only memory allocation failures throw.
Stacktraces are diagnostic aids and must not turn a diagnostic path into
a new failure path.

## Skipping frames

`current(skip)` and `current(skip, max_depth)` count from the caller of
`current`. The library's own frames are accounted for internally, so
`current(0)` starts at the function that called `current`, and
`current(1)` starts at its caller. This is what makes the idiom of
capturing in a constructor and skipping the constructor itself work.

## Thread safety

Capture is thread-safe. DbgHelp, used by the `Win32` backend, is not:
the backend serializes its own `Sym*` calls with a mutex, but cannot
protect against other code in the process that calls DbgHelp directly.
