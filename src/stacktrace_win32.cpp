// Eggs.Stacktrace
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#include <eggs/stacktrace.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <mutex>
#include <string>
#include <vector>

#include "detail/backend.hpp"

// clang-format off
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <dbghelp.h>
// clang-format on

namespace eggs {
namespace detail {

namespace {

// DbgHelp functions are not thread-safe; serialize all Sym* calls.
std::mutex& dbg_mutex() noexcept
{
    static std::mutex m;
    return m;
}

// One-time initialization: configure options and load symbols for the
// current process. Called before every Sym* API to ensure it runs first.
// SYMOPT_UNDNAME:        automatically demangle C++ names in SymFromAddr.
// SYMOPT_LOAD_LINES:     populate FileName/LineNumber in SymGetLineFromAddr64.
// SYMOPT_DEFERRED_LOADS: defer per-module symbol loading until first query.
bool dbg_init() noexcept
{
    static bool const ok = []() noexcept -> bool {
        ::SymSetOptions(
            SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES
        );
        if (::SymInitialize(::GetCurrentProcess(), nullptr, TRUE) != FALSE)
            return true;
        // DbgHelp's state is keyed by the process handle, which is the same
        // pseudo-handle for every module in this process; if some other
        // module (e.g. an EXE and a plugin DLL both statically linking this
        // backend) already called SymInitialize first, this call fails with
        // ERROR_INVALID_PARAMETER even though lookups work fine.
        return ::GetLastError() == ERROR_INVALID_PARAMETER;
    }();
    return ok;
}

// Sym* lookups only see modules that were loaded at the time DbgHelp last
// enumerated them (the initial SymInitialize call, or the last refresh
// below). Retry once after a refresh so modules loaded later (e.g. via a
// later LoadLibrary) are visible for symbolization too.
template <typename F>
auto with_module_refresh(F&& f) noexcept -> decltype(f())
{
    auto result = f();
    if (!result) {
        ::SymRefreshModuleList(::GetCurrentProcess());
        result = f();
    }
    return result;
}

} // namespace

void capture(
    std::vector<stacktrace_entry>& frames, std::size_t skip,
    std::size_t max_depth
) noexcept
{
    if (max_depth == 0) return;

    // +1 to skip this capture() frame itself. Guard against skip + 1
    // overflowing (and wrapping to a small value) when cast down to ULONG.
    constexpr std::size_t overhead = 1;
    if (skip >
        static_cast<std::size_t>(std::numeric_limits<ULONG>::max()) - overhead)
        return;
    ULONG const first = static_cast<ULONG>(skip + overhead);

    // CaptureStackBackTrace's FramesToCapture already excludes skipped
    // frames, so `got == cap` means the walk may have been truncated; grow
    // the buffer and recapture until it isn't, or a hard limit is hit.
    constexpr std::size_t kInitial = 128;
    constexpr std::size_t kHardLimit = 1 << 16;

    void* stack_buf[kInitial];
    void** buf = stack_buf;
    std::size_t cap = kInitial;
    USHORT got =
        ::CaptureStackBackTrace(first, static_cast<ULONG>(cap), buf, nullptr);

    std::vector<void*> heap_buf;
    try {
        while (static_cast<std::size_t>(got) == cap && cap < kHardLimit) {
            if (static_cast<std::size_t>(got) >= max_depth) break;

            cap *= 2;
            heap_buf.resize(cap);
            buf = heap_buf.data();
            got = ::CaptureStackBackTrace(
                first, static_cast<ULONG>(cap), buf, nullptr
            );
        }
    } catch (...) {
        return;
    }

    std::size_t const take = std::min(static_cast<std::size_t>(got), max_depth);
    try {
        for (std::size_t i = 0; i < take; ++i)
            frames.push_back(capture_helper::make(buf[i]));
    } catch (...) {
    }
}

std::string symbolize_description(void* address)
{
    if (!address || !dbg_init()) return {};

    // SYMBOL_INFO::Name[1] is at the end; extend with 255 extra bytes to hold
    // names up to 256 characters (MaxNameLen includes the null terminator).
    struct
    {
        SYMBOL_INFO sym;
        char overflow[255];
    } buf = {};

    buf.sym.SizeOfStruct = sizeof(SYMBOL_INFO);
    buf.sym.MaxNameLen = 256;

    std::lock_guard<std::mutex> lock(dbg_mutex());
    DWORD64 const addr = reinterpret_cast<DWORD64>(address);
    bool const found = with_module_refresh([&]() noexcept {
        return ::SymFromAddr(::GetCurrentProcess(), addr, nullptr, &buf.sym) !=
               FALSE;
    });
    if (!found) return {};

    // SYMOPT_UNDNAME demangled the name already.
    return buf.sym.Name;
}

// CaptureStackBackTrace stores return addresses (instruction after the call).
// Subtract 1 to point into the call instruction for accurate source info.

std::string symbolize_source_file(void* address)
{
    if (!address || !dbg_init()) return {};

    IMAGEHLP_LINE64 line = {};
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    DWORD displacement = 0;
    DWORD64 const addr = reinterpret_cast<DWORD64>(address) - 1;

    std::lock_guard<std::mutex> lock(dbg_mutex());
    bool const found = with_module_refresh([&]() noexcept {
        return ::SymGetLineFromAddr64(
                   ::GetCurrentProcess(), addr, &displacement, &line
               ) != FALSE;
    });
    if (!found) return {};

    return line.FileName ? std::string(line.FileName) : std::string{};
}

std::uint_least32_t symbolize_source_line(void* address)
{
    if (!address || !dbg_init()) return 0;

    IMAGEHLP_LINE64 line = {};
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    DWORD displacement = 0;
    DWORD64 const addr = reinterpret_cast<DWORD64>(address) - 1;

    std::lock_guard<std::mutex> lock(dbg_mutex());
    bool const found = with_module_refresh([&]() noexcept {
        return ::SymGetLineFromAddr64(
                   ::GetCurrentProcess(), addr, &displacement, &line
               ) != FALSE;
    });
    if (!found) return 0;

    return static_cast<std::uint_least32_t>(line.LineNumber);
}

} // namespace detail
} // namespace eggs
