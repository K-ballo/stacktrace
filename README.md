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

## CMake Integration

```cmake
find_package(Eggs.Stacktrace REQUIRED)
target_link_libraries(my_target PRIVATE Eggs::Stacktrace)
```

## License

Distributed under the [Boost Software License, Version 1.0](LICENSE.txt).
