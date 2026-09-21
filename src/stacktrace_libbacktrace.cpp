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
#include <exception>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "detail/backend.hpp"

#ifdef EGGS_STACKTRACE_BACKTRACE_INCLUDE_FILE
#    include EGGS_STACKTRACE_BACKTRACE_INCLUDE_FILE
#else
#    include <backtrace.h>
#endif
#include <cxxabi.h>

namespace eggs {
namespace detail {

namespace {

void on_state_error(
    void* /*unused*/, char const* /*unused*/, int /*unused*/
) noexcept
{
}

// One shared state per process; backtrace_create_state is called once.
// nullptr filename lets libbacktrace auto-detect the executable path via
// /proc/self/exe or equivalent. threaded=1 enables internal locking.
backtrace_state* bt_state() noexcept
{
    static backtrace_state* const s =
        ::backtrace_create_state(nullptr, 1, &on_state_error, nullptr);
    return s;
}

struct capture_ctx
{
    std::vector<stacktrace_entry>* frames;
    std::size_t remaining;

    static int on_pc(void* data, uintptr_t pc) noexcept
    {
        // libbacktrace terminates the walk with a sentinel (0 or UINTPTR_MAX).
        if (pc == static_cast<uintptr_t>(0) ||
            pc == std::numeric_limits<uintptr_t>::max())
            return 1;

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

    static void
    on_error(void* /*unused*/, char const* /*unused*/, int /*unused*/) noexcept
    {
    }
};

struct syminfo_ctx
{
    std::string result;
    std::exception_ptr error;

    // backtrace_syminfo reads .symtab (static symbol table)
    static void on_syminfo(
        void* data, uintptr_t /*pc*/, char const* symname, uintptr_t /*symval*/,
        uintptr_t /*symsize*/
    ) noexcept
    {
        if (symname == nullptr) return;

        auto& self = *static_cast<syminfo_ctx*>(data);
        int status = -1;
        std::unique_ptr<char, void (*)(void*)> const demangled(
            abi::__cxa_demangle(symname, nullptr, nullptr, &status), &std::free
        );
        try {
            self.result = (status == 0 && demangled != nullptr)
                              ? demangled.get()
                              : symname;
        } catch (...) {
            self.error = std::current_exception();
        }
    }

    static void
    on_error(void* /*unused*/, char const* /*unused*/, int /*unused*/) noexcept
    {
    }
};

// Collapse "./" components.
std::string normalize_slashdot(std::string p)
{
    while (p.size() >= 2 && p[0] == '.' && p[1] == '/') p.erase(0, 2);
    for (std::size_t pos = 0; (pos = p.find("/./")) != std::string::npos;)
        p.erase(pos, 2);
    return p;
}

struct pcinfo_ctx
{
    std::string file;
    std::uint_least32_t line = 0;
    std::exception_ptr error;

    // Stop after the first invocation (when the PC describes an inlined call
    // chain, libbacktrace may call this callback multiple times).
    static int on_pcinfo(
        void* data, uintptr_t /*pc*/, char const* filename, int lineno,
        char const* /*function*/
    ) noexcept
    {
        auto& self = *static_cast<pcinfo_ctx*>(data);
        try {
            self.file = filename != nullptr ? filename : "";
        } catch (...) {
            self.error = std::current_exception();
        }
        self.line = lineno > 0 ? static_cast<std::uint_least32_t>(lineno) : 0;
        return 1;
    }

    static void
    on_error(void* /*unused*/, char const* /*unused*/, int /*unused*/) noexcept
    {
    }
};

pcinfo_ctx get_pcinfo(void* address)
{
    pcinfo_ctx ctx;
    backtrace_state* const state = bt_state();
    if (state == nullptr) return ctx;

    ::backtrace_pcinfo(
        state, reinterpret_cast<uintptr_t>(address), &pcinfo_ctx::on_pcinfo,
        &pcinfo_ctx::on_error, &ctx
    );
    if (ctx.error) std::rethrow_exception(ctx.error);
    return ctx;
}

} // namespace

void capture(
    std::vector<stacktrace_entry>& frames, std::size_t skip,
    std::size_t max_depth
) noexcept
{
    if (max_depth == 0) return;

    backtrace_state* const state = bt_state();
    if (state == nullptr) return;

    // +1 to skip this capture() frame itself in the backtrace.
    constexpr std::size_t overhead = 1;
    if (skip >
        static_cast<std::size_t>(std::numeric_limits<int>::max()) - overhead)
        return;

    capture_ctx ctx{&frames, max_depth};
    ::backtrace_simple(
        state, static_cast<int>(skip + overhead), &capture_ctx::on_pc,
        &capture_ctx::on_error, &ctx
    );
}

std::string symbolize_description(void* address)
{
    if (address == nullptr) return {};

    backtrace_state* const state = bt_state();
    if (state == nullptr) return {};

    syminfo_ctx ctx;
    ::backtrace_syminfo(
        state, reinterpret_cast<uintptr_t>(address), &syminfo_ctx::on_syminfo,
        &syminfo_ctx::on_error, &ctx
    );
    if (ctx.error) std::rethrow_exception(ctx.error);
    return ctx.result;
}

std::string symbolize_source_file(void* address)
{
    if (address == nullptr) return {};

    return normalize_slashdot(get_pcinfo(address).file);
}

std::uint_least32_t symbolize_source_line(void* address)
{
    if (address == nullptr) return 0;

    return get_pcinfo(address).line;
}

} // namespace detail
} // namespace eggs
