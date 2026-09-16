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
        return ::SymInitialize(::GetCurrentProcess(), nullptr, TRUE) != FALSE;
    }();
    return ok;
}

} // namespace

void capture(
    std::vector<stacktrace_entry>& frames, std::size_t skip,
    std::size_t max_depth
) noexcept
{
    if (max_depth == 0) return;

    constexpr ULONG kMax = 128;
    void* buf[kMax];
    // +1 to skip this capture() frame itself.
    USHORT const got = ::CaptureStackBackTrace(
        static_cast<ULONG>(skip + 1),
        static_cast<ULONG>(std::min(max_depth, static_cast<std::size_t>(kMax))),
        buf, nullptr
    );
    try {
        for (USHORT i = 0; i < got; ++i)
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
    if (!::SymFromAddr(
            ::GetCurrentProcess(), reinterpret_cast<DWORD64>(address), nullptr,
            &buf.sym
        ))
        return {};

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

    std::lock_guard<std::mutex> lock(dbg_mutex());
    if (!::SymGetLineFromAddr64(
            ::GetCurrentProcess(), reinterpret_cast<DWORD64>(address) - 1,
            &displacement, &line
        ))
        return {};

    return line.FileName ? std::string(line.FileName) : std::string{};
}

std::uint_least32_t symbolize_source_line(void* address)
{
    if (!address || !dbg_init()) return 0;

    IMAGEHLP_LINE64 line = {};
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    DWORD displacement = 0;

    std::lock_guard<std::mutex> lock(dbg_mutex());
    if (!::SymGetLineFromAddr64(
            ::GetCurrentProcess(), reinterpret_cast<DWORD64>(address) - 1,
            &displacement, &line
        ))
        return 0;

    return static_cast<std::uint_least32_t>(line.LineNumber);
}

} // namespace detail
} // namespace eggs
