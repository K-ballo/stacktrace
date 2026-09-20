// Eggs.Stacktrace
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#pragma once

#if !defined(_WIN32)
#    include <dlfcn.h>
#endif

namespace eggs {
namespace detail {

// AddressSanitizer intercepts backtrace() with a wrapper, adding one extra
// frame at the top of the captured backtrace.
inline bool asan_backtrace_adds_extra_frame() noexcept
{
#if defined(__GLIBC__)
    static bool const has_asan =
        (::dlsym(RTLD_DEFAULT, "__asan_get_current_fake_stack") != nullptr);
    return has_asan;
#else
    return false;
#endif
}

} // namespace detail
} // namespace eggs
