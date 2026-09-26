// Eggs.Stacktrace
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

// dladdr() is a GNU extension; expose it before any system headers.
#ifndef _GNU_SOURCE
#    define _GNU_SOURCE
#endif

#include <eggs/stacktrace.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "detail/backend.hpp"

#ifdef EGGS_STACKTRACE_BACKTRACE_INCLUDE_FILE
#    include EGGS_STACKTRACE_BACKTRACE_INCLUDE_FILE
#else
#    include <backtrace.h>
#endif
#include <cxxabi.h>
#if defined(__has_include)
#    if __has_include(<dlfcn.h>)
#        include <dlfcn.h>
#        define EGGS_STACKTRACE_HAVE_DLADDR
#    endif
#endif
#if defined(__GLIBC__) || defined(__FreeBSD__)
#    include <link.h>
#    define EGGS_STACKTRACE_HAVE_DLPI_ADDS
#endif

namespace eggs {
namespace detail {

namespace {

void on_state_error(
    void* /*unused*/, char const* /*unused*/, int /*unused*/
) noexcept
{
}

// nullptr filename lets libbacktrace auto-detect the executable path via
// /proc/self/exe or equivalent. threaded=1 enables internal locking.
backtrace_state* create_state() noexcept
{
    return ::backtrace_create_state(nullptr, 1, &on_state_error, nullptr);
}

// How many modules have been loaded and unloaded so far, where the dynamic
// linker tells.
struct module_generation
{
    bool known = false;
    unsigned long long adds = 0;
    unsigned long long subs = 0;

    friend bool
    operator!=(module_generation const& l, module_generation const& r) noexcept
    {
        return l.adds != r.adds || l.subs != r.subs;
    }
};

#ifdef EGGS_STACKTRACE_HAVE_DLPI_ADDS
int on_phdr(::dl_phdr_info* info, std::size_t size, void* data) noexcept
{
    auto& generation = *static_cast<module_generation*>(data);
    if (size >= offsetof(::dl_phdr_info, dlpi_subs) + sizeof(info->dlpi_subs)) {
        generation.known = true;
        generation.adds = info->dlpi_adds;
        generation.subs = info->dlpi_subs;
    }
    return 1; // the same in every entry; stop at the first
}
#endif

module_generation current_module_generation() noexcept
{
    module_generation generation;
#ifdef EGGS_STACKTRACE_HAVE_DLPI_ADDS
    ::dl_iterate_phdr(&on_phdr, &generation);
#endif
    return generation;
}

// A state only knows the modules loaded when it first symbolizes, and there
// is no refreshing it, so one is recreated after modules are loaded or
// unloaded. States can't be freed either: replaced ones leak, which also
// keeps them valid for concurrent users, hence the cap.
constexpr int max_state_restarts = 16;

struct state_holder
{
    std::mutex mutex;
    backtrace_state* state = nullptr;
    module_generation generation;
    int restarts = 0;
};

state_holder& holder() noexcept
{
    static state_holder h;
    return h;
}

// A state for unwinding, which doesn't depend on the loaded modules.
backtrace_state* bt_state() noexcept
{
    state_holder& h = holder();
    std::lock_guard<std::mutex> const lock(h.mutex);
    if (h.state == nullptr) {
        h.generation = current_module_generation();
        h.state = create_state();
    }
    return h.state;
}

// A state for symbolizing, recreated if modules were loaded or unloaded
// since the current one was created.
backtrace_state* bt_symbolize_state() noexcept
{
    module_generation const now = current_module_generation();
    state_holder& h = holder();
    std::lock_guard<std::mutex> const lock(h.mutex);
    if (h.state == nullptr) {
        h.generation = now;
        h.state = create_state();
    } else if (
        now.known && now != h.generation && h.restarts < max_state_restarts
    ) {
        if (backtrace_state* const s = create_state()) {
            h.state = s;
            ++h.restarts;
        }
        h.generation = now;
    }
    return h.state;
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

std::string demangle(char const* symname)
{
    int status = -1;
    std::unique_ptr<char, void (*)(void*)> const demangled(
        abi::__cxa_demangle(symname, nullptr, nullptr, &status), &std::free
    );
    return (status == 0 && demangled != nullptr) ? demangled.get() : symname;
}

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
        try {
            self.result = demangle(symname);
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
    backtrace_state* const state = bt_symbolize_state();
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

    backtrace_state* const state = bt_symbolize_state();
    if (state == nullptr) return {};

    syminfo_ctx ctx;
    ::backtrace_syminfo(
        state, reinterpret_cast<uintptr_t>(address), &syminfo_ctx::on_syminfo,
        &syminfo_ctx::on_error, &ctx
    );
    if (ctx.error) std::rethrow_exception(ctx.error);

#ifdef EGGS_STACKTRACE_HAVE_DLADDR
    // Modules loaded after the state was created stay unknown to it where
    // loads can't be detected, or once out of restarts; the dynamic linker
    // can still name their exported symbols.
    if (ctx.result.empty()) {
        ::Dl_info info{};
        if (::dladdr(address, &info) != 0 && info.dli_sname != nullptr)
            return demangle(info.dli_sname);
    }
#endif
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
