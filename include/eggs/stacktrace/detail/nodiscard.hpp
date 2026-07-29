// Eggs.Stacktrace
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#pragma once

// EGGS_STACKTRACE_NODISCARD - compatibility macro for C++17 [[nodiscard]]
//
// Not part of the documented public API; subject to change without notice.

#ifndef EGGS_STACKTRACE_NODISCARD
#    if __cplusplus >= 201703L
#        define EGGS_STACKTRACE_NODISCARD [[nodiscard]]
#    elif defined(__GNUC__)
#        define EGGS_STACKTRACE_NODISCARD \
            __attribute__((__warn_unused_result__))
#    elif defined(_MSC_VER)
#        define EGGS_STACKTRACE_NODISCARD _Check_return_
#    else
#        define EGGS_STACKTRACE_NODISCARD
#    endif
#endif
