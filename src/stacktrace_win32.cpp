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

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
// <dbghelp.h> requires <windows.h> to be included first.
#include <dbghelp.h>

namespace eggs {
namespace detail {

namespace {

// DbgHelp is not thread-safe; serialize the Sym* calls made by this backend.
// DbgHelp state is process-global, so this does not protect against Sym*
// calls made elsewhere in the process.
std::mutex& dbg_mutex() noexcept
{
    static std::mutex m;
    return m;
}

// One-time initialization, must run before any other Sym* call.
bool dbg_init() noexcept
{
    static bool const ok = []() noexcept -> bool {
        ::SymSetOptions(
            SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES
        );
        if (::SymInitialize(::GetCurrentProcess(), nullptr, TRUE) != FALSE)
            return true;
        // Already initialized by some other module in the process.
        return ::GetLastError() == ERROR_INVALID_PARAMETER;
    }();
    return ok;
}

// Sym* lookups only see modules known at the last enumeration; retry once
// after a refresh to pick up modules loaded since.
template <typename F>
auto with_module_refresh(F const& f) noexcept -> decltype(f())
{
    auto result = f();
    if (!result) {
        ::SymRefreshModuleList(::GetCurrentProcess());
        result = f();
    }
    return result;
}

// Whether DbgHelp's `info` describes `module`, as loaded now.
bool same_image(IMAGEHLP_MODULEW64 const& info, HMODULE module) noexcept
{
    auto const base = reinterpret_cast<unsigned char const*>(module);
    auto const& dos = *reinterpret_cast<IMAGE_DOS_HEADER const*>(base);
    auto const& nt =
        *reinterpret_cast<IMAGE_NT_HEADERS const*>(base + dos.e_lfanew);
    if (info.BaseOfImage != reinterpret_cast<DWORD64>(module) ||
        info.ImageSize != nt.OptionalHeader.SizeOfImage ||
        info.TimeDateStamp != nt.FileHeader.TimeDateStamp) {
        return false;
    }

    wchar_t path[MAX_PATH];
    DWORD const len = ::GetModuleFileNameW(module, path, MAX_PATH);
    return len == 0 || len == MAX_PATH ||
           ::CompareStringOrdinal(path, -1, info.ImageName, -1, TRUE) ==
               CSTR_EQUAL;
}

// DbgHelp keeps the modules it knows about even after they are unloaded, so
// that another one later loaded at the same address would be described by
// the stale one. Drop DbgHelp's module at `addr` unless it is what is loaded
// there; the lookup then refreshes. Must be called with the lock held.
void forget_stale_module(DWORD64 addr) noexcept
{
    HANDLE const process = ::GetCurrentProcess();
    IMAGEHLP_MODULEW64 info = {};
    info.SizeOfStruct = sizeof(info);
    if (::SymGetModuleInfoW64(process, addr, &info) == FALSE) return;

    HMODULE module = nullptr;
    bool const loaded = ::GetModuleHandleExW(
                            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCWSTR>(addr), &module
                        ) != FALSE;
    if (loaded && same_image(info, module)) return;
    ::SymUnloadModule64(process, info.BaseOfImage);
}

// Captured addresses are return addresses; subtract 1 to point into the
// call instruction itself.
IMAGEHLP_LINE64 get_line_info(void* address) noexcept
{
    IMAGEHLP_LINE64 line = {};
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    if (!address || !dbg_init()) return line;

    DWORD displacement = 0;
    DWORD64 const addr = reinterpret_cast<DWORD64>(address) - 1;

    std::lock_guard<std::mutex> const lock(dbg_mutex());
    forget_stale_module(addr);
    bool const found = with_module_refresh([&]() noexcept {
        return ::SymGetLineFromAddr64(
                   ::GetCurrentProcess(), addr, &displacement, &line
               ) != FALSE;
    });
    if (!found) return IMAGEHLP_LINE64{};
    return line;
}

// Whether `symbol`, of unknown size, is the start of the function containing
// `addr`. Must be called with the DbgHelp lock held.
bool starts_function_at(DWORD64 symbol, DWORD64 addr) noexcept
{
#if defined(_M_X64) || defined(_M_ARM64)
    // Every function that makes calls, as every frame but the innermost of a
    // trace does, has unwind data giving its bounds.
    DWORD64 image_base = 0;
    PRUNTIME_FUNCTION const function =
        ::RtlLookupFunctionEntry(addr, &image_base, nullptr);
    if (function != nullptr) {
        return image_base + function->BeginAddress == symbol;
    }
#endif
    // Otherwise, only when there are no private symbols (and hence no line
    // info) covering the address, whose function DbgHelp would have found.
    IMAGEHLP_LINE64 line = {};
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    DWORD displacement = 0;
    return ::SymGetLineFromAddr64(
               ::GetCurrentProcess(), addr, &displacement, &line
           ) == FALSE;
}

} // namespace

void capture(
    std::vector<stacktrace_entry>& frames, std::size_t skip,
    std::size_t max_depth
) noexcept
{
    if (max_depth == 0) return;

    // +1 to skip this capture() frame itself in the backtrace.
    constexpr std::size_t overhead = 1;
    if (skip >
        static_cast<std::size_t>(std::numeric_limits<ULONG>::max()) - overhead)
        return;
    ULONG const first = static_cast<ULONG>(skip + overhead);

    constexpr std::size_t kInitial = 128;
    // The captured count is reported as USHORT.
    constexpr std::size_t kHardLimit = std::numeric_limits<USHORT>::max();

    void* stack_buf[kInitial];
    void** buf = stack_buf;
    std::size_t cap = kInitial;
    USHORT got =
        ::CaptureStackBackTrace(first, static_cast<ULONG>(cap), buf, nullptr);

    std::vector<void*> heap_buf;
    try {
        while (static_cast<std::size_t>(got) == cap && cap < kHardLimit) {
            if (static_cast<std::size_t>(got) >= max_depth) break;

            cap = std::min(cap * 2, kHardLimit);
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
        // Keep the frames captured so far.
    }
}

std::string symbolize_description(void* address)
{
    if (!address || !dbg_init()) return {};

    // SYMBOL_INFO::Name is a trailing array; make room for kMaxNameLen chars.
    constexpr ULONG kMaxNameLen = MAX_SYM_NAME;

    struct
    {
        SYMBOL_INFO sym;
        char overflow[kMaxNameLen - 1];
    } buf = {};

    buf.sym.SizeOfStruct = sizeof(SYMBOL_INFO);
    buf.sym.MaxNameLen = kMaxNameLen;

    std::lock_guard<std::mutex> const lock(dbg_mutex());
    DWORD64 const addr = reinterpret_cast<DWORD64>(address) - 1;
    DWORD64 displacement = 0;
    forget_stale_module(addr);
    bool const found = with_module_refresh([&]() noexcept {
        return ::SymFromAddr(
                   ::GetCurrentProcess(), addr, &displacement, &buf.sym
               ) != FALSE;
    });
    if (!found) return {};

    // Without a symbol covering the address, DbgHelp returns the nearest
    // preceding one instead: functions with internal linkage have no public
    // symbol, and MSVC replaces overlong decorated names with a hash that
    // DbgHelp skips. Reject those rather than report a neighbor's name.
    if (buf.sym.Size != 0) {
        if (displacement >= buf.sym.Size) return {};
    } else if (!starts_function_at(buf.sym.Address, addr)) {
        return {};
    }
    return buf.sym.Name;
}

std::string symbolize_source_file(void* address)
{
    IMAGEHLP_LINE64 const line = get_line_info(address);
    return line.FileName ? std::string(line.FileName) : std::string{};
}

std::uint_least32_t symbolize_source_line(void* address)
{
    IMAGEHLP_LINE64 const line = get_line_info(address);
    return static_cast<std::uint_least32_t>(line.LineNumber);
}

} // namespace detail
} // namespace eggs
