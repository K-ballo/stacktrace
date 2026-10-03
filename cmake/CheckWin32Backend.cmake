# Eggs.Stacktrace
#
# Copyright (c) 2026 Agustin Berge
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

include_guard()

include(CheckCXXSourceCompiles)

# eggs_stacktrace_check_win32_backend(<out-var>)
#
# Checks whether the win32 backend's dependencies (CaptureStackBackTrace()
# and the DbgHelp symbol APIs) are all present and link successfully.
# Sets <out-var> as an internal cache boolean. On success, also defines the
# INTERFACE library _eggs_stacktrace_win32_support.
function(eggs_stacktrace_check_win32_backend out_var)
    set(CMAKE_REQUIRED_LIBRARIES DbgHelp)
    check_cxx_source_compiles(
        "
        #include <windows.h>
        #include <dbghelp.h>
        int main()
        {
            void* buf[1];
            USHORT const n = ::CaptureStackBackTrace(0, 1, buf, nullptr);
            HANDLE const process = ::GetCurrentProcess();
            ::SymInitialize(process, nullptr, TRUE);
            ::SymRefreshModuleList(process);
            ::SymGetLineFromAddr64(process, 0, nullptr, nullptr);
            ::SymFromAddr(process, 0, nullptr, nullptr);
            ::SymCleanup(process);
            return n;
        }
        "
        ${out_var}
    )
    if(NOT ${out_var})
        return()
    endif()

    if(NOT TARGET _eggs_stacktrace_win32_support)
        add_library(_eggs_stacktrace_win32_support INTERFACE)
        target_link_libraries(_eggs_stacktrace_win32_support INTERFACE DbgHelp)
    endif()
endfunction()
