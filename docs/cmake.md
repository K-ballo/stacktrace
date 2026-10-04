<!--
Eggs.Stacktrace

Copyright (c) 2026 Agustin Berge

Distributed under the Boost Software License, Version 1.0. (See
accompanying file LICENSE.txt or copy at
http://www.boost.org/LICENSE_1_0.txt)
-->

# CMake Integration

## Building

```sh
cmake -S . -B build
cmake --build build
cmake --install build --prefix /usr/local
```

The project ships `CMakePresets.json` with configurations for GCC,
Clang, Clang with libc++, MSVC, clang-cl, and AddressSanitizer builds.
List available presets with:

```sh
cmake --list-presets
cmake --preset dev-gcc -B build/dev-gcc
cmake --build build/dev-gcc --config Debug
ctest --test-dir build/dev-gcc --build-config Debug
```

### Options

Pass options at configure time with `-D<option>=<value>`.

| Option | Subdirectory variant | Description |
|--------|----------------------|-------------|
| **`BUILD_DOCS`** (default: `ON`) | **`EGGS_STACKTRACE_BUILD_DOCS`** (default: `OFF`) | Build the documentation |
| **`BUILD_EXAMPLES`** (default: `ON`) | **`EGGS_STACKTRACE_BUILD_EXAMPLES`** (default: `OFF`) | Build example programs |
| **`BUILD_TESTING`** (default: `ON`, via CTest) | **`EGGS_STACKTRACE_BUILD_TESTING`** (default: `OFF`) | Build the test suite |
| **`ENABLE_INSTALL`** (default: `ON`) | **`EGGS_STACKTRACE_ENABLE_INSTALL`** (default: `OFF`) | Generate install rules |

Building the documentation requires a Python 3 interpreter and network
access to install the pinned `markdown` and `pygments` packages into the
build tree. Configure with `-DBUILD_DOCS=OFF` to skip it.

The prefixed variants take precedence over the short names when the
project is included via `add_subdirectory`, avoiding collisions with
the parent project's own settings.

Backend selection uses two cache variables, with no short spelling:

| Variable | Description |
|----------|-------------|
| **`EGGS_STACKTRACE_BACKENDS`** | Semicolon-separated list of backends to build, from `null`, `execinfo`, `libbacktrace`, `win32`. Empty (default) auto-detects. Requesting a backend that is not available is an error. |
| **`EGGS_STACKTRACE_DEFAULT_BACKEND`** | Backend that `Eggs::Stacktrace` aliases. Empty (default) selects the first available of `libbacktrace`, `execinfo`, `win32`, falling back to `null`. |

```sh
cmake -S . -B build \
    -DEGGS_STACKTRACE_BACKENDS="null;execinfo" \
    -DEGGS_STACKTRACE_DEFAULT_BACKEND=execinfo
```

## Consuming the library

### FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(
    eggs.stacktrace
    GIT_REPOSITORY https://github.com/eggs-cpp/stacktrace.git
    GIT_TAG        v@VERSION@
)
FetchContent_MakeAvailable(eggs.stacktrace)

target_link_libraries(my_target PRIVATE Eggs::Stacktrace)
```

### find_package (after installation)

```cmake
find_package(Eggs.Stacktrace @VERSION@ REQUIRED)
target_link_libraries(my_target PRIVATE Eggs::Stacktrace)
```

The installed package re-checks each backend's dependencies on the
consuming machine and provides only those that resolve. If the
configured default is not available, `Eggs::Stacktrace` falls back to
the `Null` backend with a warning.

---

## Targets

### `Eggs::Stacktrace`

Alias of the default backend (see `EGGS_STACKTRACE_DEFAULT_BACKEND`).
Provides `<eggs/stacktrace.hpp>` and requires C++11.

```cmake
target_link_libraries(my_target PRIVATE Eggs::Stacktrace)
```

### `Eggs::Stacktrace::Null`

Always available. Captures nothing; every trace is empty.

### `Eggs::Stacktrace::Execinfo`

POSIX systems with `backtrace()`, `dladdr()` and `__cxa_demangle()`.
Links `${CMAKE_DL_LIBS}`. Provides function names, not source
locations.

### `Eggs::Stacktrace::Libbacktrace`

Requires libbacktrace (found by the bundled `FindLibbacktrace.cmake`).
Provides function names and source locations.

### `Eggs::Stacktrace::Win32`

Windows, using `CaptureStackBackTrace` and DbgHelp. Provides function
names and source locations when PDBs are available.

All backends are static libraries and share the same public headers.
Link exactly one backend into a program; linking several defines the
same symbols more than once.
