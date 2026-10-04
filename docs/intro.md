<!--
Eggs.Stacktrace

Copyright (c) 2026 Agustin Berge

Distributed under the Boost Software License, Version 1.0. (See
accompanying file LICENSE.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
-->

# Introduction

**Eggs.Stacktrace** is a stacktrace library for C++11 and later. It
provides `eggs::stacktrace` and `eggs::stacktrace_entry`, modeled on
the C++23 `std::stacktrace` and `std::stacktrace_entry`, on compilers
and standard libraries that do not ship one -- or where the standard
one is not available at the language level you build with.

## Why

A stacktrace answers *how did we get here*. The standard library
solution requires C++23 and a library implementation that supports it.
Eggs.Stacktrace offers the same interface from C++11, with the
platform-specific capture and symbolization isolated in pluggable
backends.

## What it provides

- **`<eggs/stacktrace.hpp>`** -- `eggs::stacktrace`,
  `eggs::stacktrace_entry`, `to_string`, stream insertion, `std::hash`
  and (when available) `std::formatter` support.
- **Backends** -- `Eggs::Stacktrace::Null`, `Execinfo`, `Libbacktrace`
  and `Win32`, selected at build time. `Eggs::Stacktrace` aliases the
  best one available.

## What a stacktrace looks like

```
0x00007ff6a1b21234 in example::innermost at example/basic.cpp:35
0x00007ff6a1b21290 in example::middle at example/basic.cpp:50
0x00007ff6a1b212f0 in example::outer<42> at example/basic.cpp:66
0x00007ff6a1b2136c in main at example/basic.cpp:75
```

Each line is one entry: the address, then the description and source
location when the backend can resolve them. The exact content depends
on the backend and on whether debug information is available.

## Requirements

- A C++11 compiler (GCC 11+, Clang 17+, MSVC 2022+)
- CMake 4.0+ to build from source; CMake 3.21+ to consume an installed
  package via `find_package`
