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
// skipping an exception constructor). Whether names and source locations
// resolve is reported, never enforced.

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

// Loads the module that sits next to the executable, after the other
// scenarios have symbolized, so it is unknown when symbolization starts.
integration::core::module_forward_fn load_module(char const* argv0)
{
    std::string path = argv0;
    std::size_t const sep = path.find_last_of("/\\");
    path = (sep == std::string::npos ? std::string(".") : path.substr(0, sep)) +
           "/" + INTEGRATION_MODULE_NAME;

#if defined(_WIN32)
    HMODULE const module = ::LoadLibraryA(path.c_str());
    if (module == nullptr) return nullptr;
    FARPROC const sym = ::GetProcAddress(module, "integration_module_forward");
#else
    void* const module = ::dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (module == nullptr) return nullptr;
    void* const sym = ::dlsym(module, "integration_module_forward");
#endif
    return reinterpret_cast<integration::core::module_forward_fn>(sym);
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

    core::module_forward_fn const forward =
        argc > 0 ? load_module(argv[0]) : nullptr;
    check(forward != nullptr, "module", "load failed");
    if (forward != nullptr) {
        traced_context ctx;
        core::module_entry(ctx, forward);
        report("module", ctx);
    }

    std::printf("@result failures=%d\n", failures);
    return failures != 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
