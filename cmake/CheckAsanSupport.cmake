# Eggs.Stacktrace
#
# Copyright (c) 2026 Agustin Berge
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

include_guard()

include(CheckCXXSourceCompiles)

# eggs_stacktrace_check_asan_support(<out-var>)
#
# Checks whether the toolchain can actually compile AND link a binary with
# -fsanitize=address - i.e. that the ASan runtime is installed and usable,
# not merely that the compiler accepts the flag (which e.g. cl.exe silently
# ignores, and which a compiler package may accept without the matching
# runtime library being installed). Sets <out-var> as an internal cache
# boolean, visible to the caller.
function(eggs_stacktrace_check_asan_support out_var)
    set(CMAKE_REQUIRED_FLAGS "-fsanitize=address")
    set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=address")
    check_cxx_source_compiles(
        "
        int main()
        {
            return 0;
        }
        "
        ${out_var}
    )
endfunction()
