// Eggs.Stacktrace
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <vector>

#if defined(_MSC_VER)
#    define INTEGRATION_NOINLINE __declspec(noinline)
#else
#    define INTEGRATION_NOINLINE __attribute__((noinline))
#endif

#if defined(_WIN32)
#    define INTEGRATION_EXPORT __declspec(dllexport)
#    define INTEGRATION_IMPORT __declspec(dllimport)
#else
#    define INTEGRATION_EXPORT __attribute__((visibility("default")))
#    define INTEGRATION_IMPORT
#endif

namespace integration {

// A call site the trace is expected to go through: the frame belonging to
// function `name` should resolve to that name, and (when `line` is not 0)
// to a source location near `file`:`line`.
struct anchor
{
    char const* name;
    char const* file;
    int line;
};

// Passed down every call chain. The plugin and module never link the
// stacktrace library; their frames end up in the middle of a trace by
// calling back into the app through `callback`.
struct context
{
    std::vector<anchor> anchors;

    void note(char const* name, char const* file, int line)
    {
        anchors.push_back(anchor{name, file, line});
    }
};

using callback = void (*)(context&);

} // namespace integration

// Records the enclosing function `name` as an anchor at this line, then
// evaluates `call`.
#define INTEGRATION_CALL(ctx, name, call) \
    ((ctx).note(name, __FILE__, __LINE__), call)
