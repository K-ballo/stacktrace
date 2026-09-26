# Eggs.Stacktrace

**Eggs.Stacktrace** is a stacktrace library.

## Requirements

- CMake 4.0+ to build from source; CMake 3.21+ to consume an installed package via `find_package`
- A C++11 compiler (GCC 11+, Clang 17+, MSVC 2022+)

## Building

```sh
cmake --preset dev-gcc -B build/dev-gcc
cmake --build build/dev-gcc --config Debug
ctest --test-dir build/dev-gcc --build-config Debug
```

Replace `dev-gcc` with `dev-clang`, `dev-clang-libcxx`, or `dev-msvc` as appropriate.

## Backends

| Target                       | Platform | Description                               |
|------------------------------|----------|-------------------------------------------|
| `Eggs::Stacktrace::Null`     | All      | Empty capture; always available           |
| `Eggs::Stacktrace::Execinfo` | POSIX    | *(planned)* `backtrace()` + `dladdr()` symbolization  |
| `Eggs::Stacktrace::Libbacktrace` | Linux | *(planned)* Full source file and line info via libbacktrace |
| `Eggs::Stacktrace::Win32`    | Windows  | *(planned)* `CaptureStackBackTrace` + DbgHelp         |

`Eggs::Stacktrace` aliases the best available backend on the current platform.

> **Note:** DbgHelp is not thread-safe. The win32 backend serializes its own
> `Sym*` calls, but not those made elsewhere in the process.

## Usage

```cpp
#include <eggs/stacktrace.hpp>
#include <iostream>

void foo()
{
    auto st = eggs::stacktrace::current();
    std::cout << st;
}
```

### `std::stacktrace` when available

`<stdeggs/stacktrace.hpp>` (target `Eggs::Stacktrace::Std`) provides
`stdeggs::stacktrace` and `stdeggs::stacktrace_entry`, which alias the
`std::` types when `__cpp_lib_stacktrace` is defined and the `eggs::` types
(default backend) otherwise. The target also links whatever the standard
library needs for `std::stacktrace` (e.g. `stdc++exp` on libstdc++).

Define `EGGS_STACKTRACE_STD_USE_EGGS` to always use the `eggs::` types, e.g.
when translation units built with different C++ standards share these types.

```cpp
#include <stdeggs/stacktrace.hpp>
#include <iostream>

void foo()
{
    auto st = stdeggs::stacktrace::current();
    std::cout << st;
}
```

## CMake Integration

```cmake
find_package(Eggs.Stacktrace REQUIRED)
target_link_libraries(my_target PRIVATE Eggs::Stacktrace)
```

## License

Distributed under the [Boost Software License, Version 1.0](LICENSE.txt).
