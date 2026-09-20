# Eggs.Stacktrace
#
# Copyright (c) 2026 Agustin Berge
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

include_guard()

include(CheckCXXSourceCompiles)

# eggs_stacktrace_check_execinfo_backend(<out-var>)
#
# Checks whether the execinfo backend's dependencies (backtrace(),
# dladdr(), and __cxa_demangle()) are all present and link successfully
# against CMAKE_DL_LIBS, not merely that <execinfo.h> can be included.
# Sets <out-var> as an internal cache boolean, visible to the caller. On
# success, also defines the INTERFACE library
# _eggs_stacktrace_execinfo_support, carrying CMAKE_DL_LIBS as a usage
# requirement.
function(eggs_stacktrace_check_execinfo_backend out_var)
    set(CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
    set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_DL_LIBS})
    check_cxx_source_compiles(
        "
        #include <cxxabi.h>
        #include <dlfcn.h>
        #include <execinfo.h>
        int main()
        {
            void* buf[1];
            int const n = ::backtrace(buf, 1);
            ::Dl_info info{};
            ::dladdr(buf[0], &info);
            int status = -1;
            ::abi::__cxa_demangle(\"x\", nullptr, nullptr, &status);
            return n;
        }
        "
        ${out_var}
    )
    if(${out_var} AND NOT TARGET _eggs_stacktrace_execinfo_support)
        add_library(_eggs_stacktrace_execinfo_support INTERFACE)
        target_link_libraries(
            _eggs_stacktrace_execinfo_support
            INTERFACE ${CMAKE_DL_LIBS}
        )
    endif()
endfunction()
