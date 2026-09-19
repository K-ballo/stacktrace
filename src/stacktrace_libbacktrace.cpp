// Eggs.Stacktrace
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

#include <eggs/stacktrace.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

#include "detail/backend.hpp"

#include <backtrace.h>
#include <cxxabi.h>

namespace eggs {
namespace detail {

namespace {

// One shared state per process; backtrace_create_state is called once.
// nullptr filename lets libbacktrace auto-detect the executable path via
// /proc/self/exe or equivalent. threaded=1 enables internal locking.
backtrace_state* bt_state() noexcept
{
    static backtrace_state* const s =
        ::backtrace_create_state(nullptr, 1, nullptr, nullptr);
    return s;
}

struct capture_ctx
{
    std::vector<stacktrace_entry>* frames;
    std::size_t remaining;

    static int on_pc(void* data, uintptr_t pc) noexcept
    {
        // libbacktrace terminates the walk with a sentinel (0 or UINTPTR_MAX).
        if (pc == 0 || pc == static_cast<uintptr_t>(-1)) return 1;

        auto& self = *static_cast<capture_ctx*>(data);
        if (self.remaining == 0) return 1;
        try {
            self.frames->push_back(
                capture_helper::make(reinterpret_cast<void*>(pc))
            );
        } catch (...) {
            return 1;
        }
        return --self.remaining == 0 ? 1 : 0;
    }

    static void on_error(void*, char const*, int) noexcept {}
};

struct syminfo_ctx
{
    std::string result;

    // backtrace_syminfo reads .symtab (static symbol table), so it resolves
    // names regardless of -rdynamic or -fvisibility=hidden.
    static void on_syminfo(
        void* data, uintptr_t, char const* symname, uintptr_t, uintptr_t
    ) noexcept
    {
        if (!symname) return;

        auto& self = *static_cast<syminfo_ctx*>(data);
        int status = -1;
        char* demangled =
            abi::__cxa_demangle(symname, nullptr, nullptr, &status);
        try {
            self.result = (status == 0 && demangled) ? demangled : symname;
        } catch (...) {
        }
        std::free(demangled);
    }

    static void on_error(void*, char const*, int) noexcept {}
};

// Collapse /./ sequences that appear when libbacktrace joins DW_AT_comp_dir
// with DW_AT_name: Clang records the build directory as comp_dir, which after
// -ffile-prefix-map becomes ".", producing "././example/foo.cpp".
std::string normalize_slashdot(std::string p)
{
    for (std::size_t pos; (pos = p.find("/./")) != std::string::npos;)
        p.erase(pos, 2);
    return p;
}

struct pcinfo_ctx
{
    std::string file;
    std::uint_least32_t line = 0;

    static int on_pcinfo(
        void* data, uintptr_t, char const* filename, int lineno, char const*
    ) noexcept
    {
        auto& self = *static_cast<pcinfo_ctx*>(data);
        try {
            self.file = filename ? filename : "";
        } catch (...) {
        }
        self.line = lineno > 0 ? static_cast<std::uint_least32_t>(lineno) : 0;
        return 0;
    }

    static void on_error(void*, char const*, int) noexcept {}
};

} // namespace

void capture(
    std::vector<stacktrace_entry>& frames, std::size_t skip,
    std::size_t max_depth
) noexcept
{
    if (max_depth == 0) return;

    capture_ctx ctx{&frames, max_depth};
    // +1 to skip this capture() frame itself. backtrace_simple's skip=N means
    // "skip N frames starting from capture()", where N=0 is capture() itself.
    ::backtrace_simple(
        bt_state(), static_cast<int>(skip + 1), &capture_ctx::on_pc,
        &capture_ctx::on_error, &ctx
    );
}

std::string symbolize_description(void* address)
{
    if (!address) return {};

    syminfo_ctx ctx;
    ::backtrace_syminfo(
        bt_state(), reinterpret_cast<uintptr_t>(address),
        &syminfo_ctx::on_syminfo, &syminfo_ctx::on_error, &ctx
    );
    return ctx.result;
}

std::string symbolize_source_file(void* address)
{
    if (!address) return {};

    pcinfo_ctx ctx;
    ::backtrace_pcinfo(
        bt_state(), reinterpret_cast<uintptr_t>(address),
        &pcinfo_ctx::on_pcinfo, &pcinfo_ctx::on_error, &ctx
    );
    return normalize_slashdot(std::move(ctx.file));
}

std::uint_least32_t symbolize_source_line(void* address)
{
    if (!address) return 0;

    pcinfo_ctx ctx;
    ::backtrace_pcinfo(
        bt_state(), reinterpret_cast<uintptr_t>(address),
        &pcinfo_ctx::on_pcinfo, &pcinfo_ctx::on_error, &ctx
    );
    return ctx.line;
}

} // namespace detail
} // namespace eggs
