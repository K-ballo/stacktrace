// Eggs.Stacktrace
//
// Copyright (c) 2026 Agustin Berge
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt

// Runs every scenario and prints its trace, followed by machine-readable
// lines that run_matrix.py collects:
//
//   @anchor <scenario> <name> sym=<0|1> loc=<0|1|->
//   @scenario <scenario> frames=<n> sym=<hits>/<total> loc=<hits>/<total>
//
// Checks are deliberately minimal: a non-null backend must capture a
// non-empty trace, a capped capture must respect its cap, and frame 0, when
// named, must be the function that captured (or the throw site, for a capture
// skipping the exception's factory), and no frame may be named after a
// function of an unloaded module. Whether names and source locations resolve
// is reported, never enforced.

#include <eggs/stacktrace.hpp>

#include <cctype>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include <integration/core.hpp>

#if defined(_WIN32)
#    define NOMINMAX
#    define WIN32_LEAN_AND_MEAN
#    include <windows.h>
#else
#    include <dlfcn.h>
#endif

namespace {

using integration::core::traced_context;

bool const is_null_backend = std::strcmp(INTEGRATION_BACKEND, "null") == 0;
int failures = 0;

std::string file_name(std::string const& path)
{
    std::size_t const sep = path.find_last_of("/\\");
    return sep == std::string::npos ? path : path.substr(sep + 1);
}

bool same_file(std::string const& lhs, std::string const& rhs)
{
    std::string const l = file_name(lhs);
    std::string const r = file_name(rhs);
#if defined(_WIN32)
    if (l.size() != r.size()) return false;
    for (std::size_t i = 0; i < l.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(l[i])) !=
            std::tolower(static_cast<unsigned char>(r[i]))) {
            return false;
        }
    }
    return true;
#else
    return l == r;
#endif
}

void check(bool cond, char const* scenario, char const* what)
{
    if (!cond) {
        std::printf("@fail %s %s\n", scenario, what);
        ++failures;
    }
}

void report(char const* scenario, traced_context const& ctx)
{
    std::printf("=== %s\n%s\n", scenario, eggs::to_string(ctx.trace).c_str());

    int sym_hits = 0;
    int loc_hits = 0;
    int loc_total = 0;
    for (integration::anchor const& a : ctx.anchors) {
        bool sym = false;
        bool loc = false;
        for (eggs::stacktrace_entry const& e : ctx.trace) {
            if (e.description().find(a.name) != std::string::npos) sym = true;
            // Return addresses usually map to the call's line, but may land
            // on a neighbor when a call expression spans several lines.
            long const line = static_cast<long>(e.source_line());
            if (a.line != 0 && line >= a.line - 3 && line <= a.line + 3 &&
                same_file(e.source_file(), a.file)) {
                loc = true;
            }
        }
        sym_hits += sym ? 1 : 0;
        loc_hits += loc ? 1 : 0;
        loc_total += a.line != 0 ? 1 : 0;
        std::printf(
            "@anchor %s %s sym=%d loc=%s\n", scenario, a.name, sym ? 1 : 0,
            a.line == 0 ? "-"
            : loc       ? "1"
                        : "0"
        );
    }
    std::printf(
        "@scenario %s frames=%zu sym=%d/%zu loc=%d/%d\n", scenario,
        ctx.trace.size(), sym_hits, ctx.anchors.size(), loc_hits, loc_total
    );

    if (!is_null_backend) check(!ctx.trace.empty(), scenario, "empty trace");
    for (eggs::stacktrace_entry const& e : ctx.trace) {
        std::string const name = e.description();
        for (char const* stale : ctx.stale) {
            if (name.find(stale) != std::string::npos) {
                std::printf(
                    "@fail %s frame named after unloaded %s\n", scenario, stale
                );
                ++failures;
            }
        }
    }
    // Frame 0 need not have a name, but a name must be the right one.
    if (ctx.top != nullptr && !ctx.trace.empty()) {
        std::string const top = ctx.trace[0].description();
        if (!top.empty() && top.find(ctx.top) == std::string::npos) {
            std::printf(
                "@fail %s frame 0 is \"%.120s\", expected %s\n", scenario,
                top.c_str(), ctx.top
            );
            ++failures;
        }
    }
    if (ctx.max_frames != 0) {
        check(ctx.trace.size() <= ctx.max_frames, scenario, "exceeds max");
    }
}

void run(char const* scenario, void (*entry)(traced_context&))
{
    traced_context ctx;
    entry(ctx);
    report(scenario, ctx);
}

struct loaded_module
{
    std::string path;
    void* handle = nullptr;
    integration::core::module_forward_fn forward = nullptr;
};

// Loads a module that sits next to the executable, and looks up its entry
// point `forward`.
loaded_module
load_module(char const* argv0, char const* name, char const* forward)
{
    loaded_module m;
    m.path = argv0;
    std::size_t const sep = m.path.find_last_of("/\\");
    m.path =
        (sep == std::string::npos ? std::string(".") : m.path.substr(0, sep)) +
        "/" + name;

#if defined(_WIN32)
    HMODULE const module = ::LoadLibraryA(m.path.c_str());
    if (module == nullptr) return m;
    m.handle = module;
    FARPROC const sym = ::GetProcAddress(module, forward);
#else
    m.handle = ::dlopen(m.path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (m.handle == nullptr) return m;
    void* const sym = ::dlsym(m.handle, forward);
#endif
    m.forward = reinterpret_cast<integration::core::module_forward_fn>(sym);
    return m;
}

// Unloads `m`, returning whether it is actually gone.
bool unload_module(loaded_module const& m)
{
#if defined(_WIN32)
    ::FreeLibrary(static_cast<HMODULE>(m.handle));
    return ::GetModuleHandleA(m.path.c_str()) == nullptr;
#else
    ::dlclose(m.handle);
    void* const still = ::dlopen(m.path.c_str(), RTLD_NOW | RTLD_NOLOAD);
    if (still != nullptr) ::dlclose(still);
    return still == nullptr;
#endif
}

// The address `m` is loaded at.
void const* module_base(loaded_module const& m)
{
#if defined(_WIN32)
    return m.handle;
#else
    ::Dl_info info{};
    if (::dladdr(reinterpret_cast<void*>(m.forward), &info) == 0) {
        return nullptr;
    }
    return info.dli_fbase;
#endif
}

} // namespace

int main(int argc, char* argv[])
{
    namespace core = integration::core;

    std::printf("backend: %s\n", INTEGRATION_BACKEND);

    // First, so that the first symbolization happens concurrently.
    run("thread", &core::thread_entry);
    run("chain", &core::chain_entry);
    run("exception", &core::exception_entry);
    run("unwind", &core::unwind_entry);
    run("qsort", &core::qsort_entry);
    run("function", &core::function_entry);
    run("long_name", &core::long_name_entry);
    run("cold", &core::cold_entry);
    run("recursion", &core::recursion_entry);

    // Loaded after the scenarios above have symbolized, so it is unknown
    // when symbolization starts.
    char const* const argv0 = argc > 0 ? argv[0] : ".";
    loaded_module const a = load_module(
        argv0, INTEGRATION_MODULE_NAME, "integration_module_forward"
    );
    check(a.forward != nullptr, "module", "load failed");
    if (a.forward != nullptr) {
        traced_context ctx;
        core::module_entry(ctx, a.forward);
        report("module", ctx);

        // Replaced by another module, now that it has been symbolized; if
        // that one lands at the same address, stale information about the
        // first one would name its functions instead.
        void const* const a_base = module_base(a);
        bool const unloaded = unload_module(a);
        loaded_module const b = load_module(
            argv0, INTEGRATION_MODULE_B_NAME, "integration_module_b_forward"
        );
        check(b.forward != nullptr, "reload", "load failed");
        if (b.forward != nullptr) {
            std::printf(
                "@info reload unloaded=%d same_base=%d\n", unloaded ? 1 : 0,
                module_base(b) == a_base ? 1 : 0
            );
            traced_context reload;
            reload.stale = {"integration_module_forward", "module_hidden_hop"};
            core::module_entry(reload, b.forward);
            report("reload", reload);
        }
    }

    std::printf("@result failures=%d\n", failures);
    return failures != 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
