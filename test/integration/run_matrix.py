#!/usr/bin/env python3
# Eggs.Stacktrace
#
# Copyright (c) 2026 Agustin Berge
#
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt

"""Build the integration consumer under every combination of flags that can
affect a stacktrace, and collect the traces.

The consumer is built against an installed Release package once per
combination of compile/link flags. Each build is then run under several
post-link variants (as built, stripped, PDB hidden, ...), once per
installed backend.

Checks are minimal: a run fails only if the combination does not build (or
is unsupported by the toolchain, unless --allow-unsupported), the installed
package lacks an expected backend, the app crashes, or a non-null backend
captures an empty trace. Symbol and source-location hit rates are reported
in summary.csv, never enforced. Failures are printed at the end, as error
annotations when running on GitHub Actions.

Output layout (under --out):
  traces/<combo>/flags.txt, configure.log, build.log
  traces/<combo>/<variant>/<backend>.txt
  summary.csv, summary.md
"""

import argparse
import concurrent.futures
import contextlib
import csv
import dataclasses
import itertools
import os
import pathlib
import re
import shlex
import shutil
import subprocess
import sys
import threading

SOURCE_DIR = pathlib.Path(__file__).resolve().parent

FAMILIES = {
    "gcc": "gnu",
    "clang": "gnu",
    "apple-clang": "apple",
    "msvc": "msvc",
    "clang-cl": "msvc",
}

# Backends the installed package must provide, besides null.
EXPECTED_BACKENDS = {
    "gcc": ["execinfo", "libbacktrace"],
    "clang": ["execinfo", "libbacktrace"],
    "apple-clang": ["execinfo"],
    "msvc": ["win32"],
    "clang-cl": ["win32"],
}

FAILED_STATUSES = {"configure-failed", "build-failed", "check-failed",
                   "crashed", "timeout", "variant-failed", "missing-backend"}


@dataclasses.dataclass(frozen=True)
class Value:
    name: str
    cxx: tuple = ()  # compile flags
    link: tuple = ()  # link flags, every binary
    exe_link: tuple = ()  # link flags, executables only
    cmake: tuple = ()  # extra -D arguments


# -- Axes ----------------------------------------------------------------------
# Each family declares its axes, the rules that prune meaningless
# combinations, and the post-link variants that apply to a combination.
#
# Only flags and variants that were seen to change a trace are kept. Trimmed
# after producing traces identical to a kept one (modulo addresses):
#   gnu:   -fomit-frame-pointer, -fPIE/-no-pie, lld, -g1, -gsplit-dwarf, -gz,
#          debuglink (== as-built), LTO with clang
#   msvc:  /Zi (== /Z7), exe moved away from its PDB (== as-built),
#          /OPT:ICF and /INCREMENTAL with clang-cl
#   apple: every debug-info flag and variant (execinfo reads no DWARF), LTO

LTO = Value("lto", cmake=("-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON",))


def gnu_axes(args):
    return {
        "opt": [Value("O0", cxx=("-O0",)), Value("O3", cxx=("-O3",))],
        "debug": [Value("g0", cxx=("-g0",)), Value("g", cxx=("-g",))],
        "exports": [
            Value("rdynamic", exe_link=("-rdynamic",)),
            Value("nordynamic"),
        ],
        "lto": [Value("nolto"), LTO],
    }


def gnu_valid(args, c):
    if c["lto"] == "lto" and (c["opt"] == "O0" or args.toolchain == "clang"):
        return False
    return True


def gnu_variants(c):
    if c["debug"] == "g0":
        return ["as-built", "strip-all"]
    return ["as-built", "strip-debug", "strip-all"]


def apple_axes(args):
    return {"opt": [Value("O0", cxx=("-O0",)), Value("O3", cxx=("-O3",))]}


def apple_valid(args, c):
    return True


def apple_variants(c):
    return ["as-built"]


def msvc_axes(args):
    inline = ("/Ob3",) if args.toolchain == "msvc" else ()
    return {
        "opt": [Value("Od", cxx=("/Od",)), Value("O2", cxx=("/O2",) + inline)],
        "debug": [
            Value("nodi"),
            Value("Z7", cmake=("-DCMAKE_MSVC_DEBUG_INFORMATION_FORMAT=Embedded",)),
        ],
        "pdb": [Value("nopdb"), Value("pdb", link=("/DEBUG:FULL",))],
        "icf": [
            Value("icf", link=("/OPT:REF", "/OPT:ICF")),
            Value("noicf", link=("/OPT:NOREF", "/OPT:NOICF")),
        ],
        "incremental": [
            Value("noinc", link=("/INCREMENTAL:NO",)),
            Value("inc", link=("/INCREMENTAL",)),
        ],
        "lto": [Value("nolto"), LTO],
    }


def msvc_valid(args, c):
    if c["opt"] == "Od" and c["lto"] == "lto":
        return False
    if args.toolchain == "clang-cl" and (
        c["icf"] == "noicf" or c["incremental"] == "inc"
    ):
        return False
    # Incremental linking needs /DEBUG and is incompatible with /OPT.
    if c["incremental"] == "inc" and (c["pdb"] != "pdb" or c["icf"] == "icf"):
        return False
    return True


def msvc_variants(c):
    if c["pdb"] == "nopdb":
        return ["as-built"]
    return ["as-built", "no-pdb"]


FAMILY_RULES = {
    "gnu": (gnu_axes, gnu_valid, gnu_variants),
    "apple": (apple_axes, apple_valid, apple_variants),
    "msvc": (msvc_axes, msvc_valid, msvc_variants),
}


def combinations(args, family):
    axes_fn, valid_fn, _ = FAMILY_RULES[family]
    axes = axes_fn(args)
    for values in itertools.product(*axes.values()):
        combo = dict(zip(axes.keys(), values))
        if valid_fn(args, {k: v.name for k, v in combo.items()}):
            yield combo


def combo_name(combo):
    return "-".join(v.name for v in combo.values())


# -- Post-link variants --------------------------------------------------------


def is_binary(path):
    with open(path, "rb") as f:
        magic = f.read(4)
    return magic in (
        b"\x7fELF",
        b"\xcf\xfa\xed\xfe",  # Mach-O 64-bit
        b"\xca\xfe\xba\xbe",  # Mach-O universal
    ) or magic[:2] == b"MZ"


def copy_binaries(src, dst, exclude_suffixes):
    if dst.exists():
        shutil.rmtree(dst)
    dst.mkdir(parents=True)
    for f in src.iterdir():
        if f.is_file() and f.suffix.lower() not in exclude_suffixes:
            shutil.copy2(f, dst / f.name)
    return [f for f in dst.iterdir() if f.is_file() and is_binary(f)]


@contextlib.contextmanager
def hidden(paths):
    """Temporarily renames `paths` so that tools looking for them fail."""
    moved = []
    try:
        for p in paths:
            h = p.with_name(p.name + ".hidden")
            p.rename(h)
            moved.append((p, h))
        yield
    finally:
        for p, h in reversed(moved):
            h.rename(p)


def tool(cmd, cwd=None):
    subprocess.run(cmd, cwd=cwd, check=True, capture_output=True)


@contextlib.contextmanager
def variant_dir(args, family, variant, build_dir):
    """Yields the directory to run the apps from for `variant`."""
    bin_dir = build_dir / "bin"
    if variant == "as-built":
        yield bin_dir
        return

    dst = build_dir / "variants" / variant
    if family == "gnu":
        for f in copy_binaries(bin_dir, dst, set()):
            if variant == "strip-debug":
                tool([args.strip, "--strip-debug", f.name], cwd=dst)
            elif variant == "strip-all":
                tool([args.strip, "--strip-all", f.name], cwd=dst)
        yield dst
    elif family == "msvc" and variant == "no-pdb":
        # Copied away from their PDBs, which are also hidden, so that DbgHelp
        # finds them neither next to the binaries nor through the absolute
        # path recorded in each binary.
        copy_binaries(bin_dir, dst, {".pdb", ".ilk", ".lib", ".exp"})
        with hidden(list(bin_dir.glob("*.pdb"))):
            yield dst
    else:
        raise ValueError(f"unknown variant {variant} for {family}")


# -- Running -------------------------------------------------------------------

SCENARIO_RE = re.compile(
    r"^@scenario (\S+) frames=(\d+) sym=(\d+)/(\d+) loc=(\d+)/(\d+)", re.M
)


def run_app(args, app, run_dir, log):
    try:
        r = subprocess.run(
            [str(run_dir / app)],
            cwd=run_dir,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            timeout=args.timeout,
        )
        output = r.stdout.decode(errors="replace")
        status = {0: "ok", 1: "check-failed"}.get(r.returncode, "crashed")
        returncode = r.returncode
    except subprocess.TimeoutExpired as e:
        output = (e.stdout or b"").decode(errors="replace")
        status, returncode = "timeout", None
    log.write_text(output, encoding="utf-8")

    totals = dict(scenarios=0, frames=0, sym_hits=0, sym_total=0,
                  loc_hits=0, loc_total=0)
    for m in SCENARIO_RE.finditer(output):
        totals["scenarios"] += 1
        totals["frames"] += int(m.group(2))
        totals["sym_hits"] += int(m.group(3))
        totals["sym_total"] += int(m.group(4))
        totals["loc_hits"] += int(m.group(5))
        totals["loc_total"] += int(m.group(6))
    fails = re.findall(r"^@fail (.*)$", output, re.M)
    details = ("; ".join(fails) if fails
               else f"exit code {returncode}" if status == "crashed" else "")
    return dict(status=status, returncode=returncode, details=details,
                **totals)


def run_logged(cmd, log, cwd=None):
    r = subprocess.run(cmd, cwd=cwd, stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT)
    output = r.stdout.decode(errors="replace")
    log.write_text(shlex.join(cmd) + "\n\n" + output, encoding="utf-8")
    return r.returncode == 0, output


def app_backend(app):
    return app[len("integration_app_"):].removesuffix(".exe")

def run_combo(args, family, combo):
    name = combo_name(combo)
    build_dir = args.out / "builds" / name
    trace_dir = args.out / "traces" / name
    trace_dir.mkdir(parents=True, exist_ok=True)
    axes = {k: v.name for k, v in combo.items()}
    row = dict(combo=name, **axes)

    def flags(field):
        return " ".join(f for v in combo.values() for f in getattr(v, field))

    link = flags("link")
    exe_link = " ".join(filter(None, [link, flags("exe_link")]))
    cmd = [
        "cmake", "-S", str(SOURCE_DIR), "-B", str(build_dir), "-G", "Ninja",
        "-DCMAKE_BUILD_TYPE=Integration",
        f"-DCMAKE_PREFIX_PATH={args.prefix}",
        f"-DCMAKE_CXX_FLAGS_INTEGRATION={flags('cxx')}",
        f"-DCMAKE_EXE_LINKER_FLAGS_INTEGRATION={exe_link}",
        f"-DCMAKE_SHARED_LINKER_FLAGS_INTEGRATION={link}",
        f"-DCMAKE_MODULE_LINKER_FLAGS_INTEGRATION={link}",
    ]
    if args.cxx:
        cmd.append(f"-DCMAKE_CXX_COMPILER={args.cxx}")
    cmd += [a for v in combo.values() for a in v.cmake]
    (trace_dir / "flags.txt").write_text(
        "\n".join(f"{k}: {v}" for k, v in axes.items()) + "\n", "utf-8"
    )

    def log_path(*parts):
        return (trace_dir.joinpath(*parts)).relative_to(args.out).as_posix()

    ok, output = run_logged(cmd, trace_dir / "configure.log")
    if not ok:
        status = ("unsupported" if "INTEGRATION_UNSUPPORTED" in output
                  else "configure-failed")
        return [dict(row, variant="-", backend="-", status=status,
                     log=log_path("configure.log"))]

    ok, _ = run_logged(
        ["cmake", "--build", str(build_dir), "-j", str(args.build_jobs)],
        trace_dir / "build.log",
    )
    if not ok:
        return [dict(row, variant="-", backend="-", status="build-failed",
                     log=log_path("build.log"))]

    apps = sorted(
        f.name for f in (build_dir / "bin").iterdir()
        if f.is_file() and f.name.startswith("integration_app_")
        and f.suffix in ("", ".exe")
    )
    rows = [
        dict(row, variant="-", backend=b, status="missing-backend",
             details="not provided by the installed package",
             log=log_path("configure.log"))
        for b in args.expect_backends
        if b not in {app_backend(a) for a in apps}
    ]
    _, _, variants_fn = FAMILY_RULES[family]
    for variant in variants_fn(axes):
        (trace_dir / variant).mkdir(exist_ok=True)
        try:
            with variant_dir(args, family, variant, build_dir) as run_dir:
                for app in apps:
                    backend = app_backend(app)
                    log = trace_dir / variant / f"{backend}.txt"
                    result = run_app(args, app, run_dir, log)
                    rows.append(dict(row, variant=variant, backend=backend,
                                     log=log_path(variant, f"{backend}.txt"),
                                     **result))
        except (subprocess.CalledProcessError, OSError) as e:
            (trace_dir / variant / "variant.log").write_text(str(e), "utf-8")
            rows.append(dict(row, variant=variant, backend="-",
                             status="variant-failed", details=str(e),
                             log=log_path(variant, "variant.log")))

    if not args.keep_builds:
        shutil.rmtree(build_dir, ignore_errors=True)
    return rows


# -- Reporting -----------------------------------------------------------------


def write_summary(args, axis_names, rows):
    fields = (["combo"] + axis_names + ["variant", "backend", "status",
              "returncode", "scenarios", "frames", "sym_hits", "sym_total",
              "loc_hits", "loc_total", "log"])
    with open(args.out / "summary.csv", "w", newline="",
              encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=fields, restval="",
                           extrasaction="ignore")
        w.writeheader()
        w.writerows(rows)

    counts = {}
    for r in rows:
        counts[r["status"]] = counts.get(r["status"], 0) + 1
    failed_statuses = FAILED_STATUSES | (
        set() if args.allow_unsupported else {"unsupported"}
    )
    failed = [r for r in rows if r["status"] in failed_statuses]

    lines = [f"## Integration matrix: {args.toolchain}", ""]
    lines += ["| status | runs |", "|---|---|"]
    lines += [f"| {s} | {n} |" for s, n in sorted(counts.items())]
    if failed:
        lines += ["", "### Failures", "",
                  "| combo | variant | backend | status | details | log |",
                  "|---|---|---|---|---|---|"]
        lines += [f"| {r['combo']} | {r['variant']} | {r['backend']} | "
                  f"{r['status']} | {r.get('details', '')} | "
                  f"{r.get('log', '')} |" for r in failed]
    text = "\n".join(lines) + "\n"
    (args.out / "summary.md").write_text(text, encoding="utf-8")
    step_summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if step_summary:
        with open(step_summary, "a", encoding="utf-8") as f:
            f.write(text)
    return failed


def report_failures(args, failed):
    """Prints one line per failure; as error annotations on GitHub Actions."""
    in_actions = os.environ.get("GITHUB_ACTIONS") == "true"
    for r in failed:
        message = (f"{r['combo']} / {r['variant']} / {r['backend']}: "
                   f"{r['status']}")
        if r.get("details"):
            message += f" ({r['details']})"
        if r.get("log"):
            message += f"; see {r['log']} in integration-traces"
        if in_actions:
            escaped = (message.replace("%", "%25").replace("\r", "%0D")
                       .replace("\n", "%0A"))
            print(f"::error title=integration {args.toolchain}::{escaped}")
        else:
            print(f"FAILED {message}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--toolchain", required=True, choices=FAMILIES)
    parser.add_argument("--prefix", type=pathlib.Path,
                        help="install prefix of the Release package")
    parser.add_argument("--out", type=pathlib.Path, default=pathlib.Path(
                        "build/integration"))
    parser.add_argument("--cxx", help="C++ compiler")
    parser.add_argument("--expect-backends",
                        help="space-separated backends the package must "
                        "provide; defaults per toolchain")
    parser.add_argument("--allow-unsupported", action="store_true",
                        help="don't fail combinations the toolchain can't "
                        "build (e.g. LTO unavailable)")
    parser.add_argument("--strip", default="strip")
    parser.add_argument("--filter", default="",
                        help="regex; only combinations whose name matches")
    parser.add_argument("--jobs", type=int,
                        default=max(1, (os.cpu_count() or 2) // 2),
                        help="combinations built concurrently")
    parser.add_argument("--build-jobs", type=int, default=2,
                        help="parallel jobs within one build")
    parser.add_argument("--timeout", type=int, default=120)
    parser.add_argument("--keep-builds", action="store_true")
    parser.add_argument("--list", action="store_true",
                        help="list combinations and exit")
    args = parser.parse_args()

    args.expect_backends = (EXPECTED_BACKENDS[args.toolchain]
                            if args.expect_backends is None
                            else args.expect_backends.split())
    family = FAMILIES[args.toolchain]
    pattern = re.compile(args.filter)
    combos = [c for c in combinations(args, family)
              if pattern.search(combo_name(c))]
    if args.list:
        for c in combos:
            print(combo_name(c))
        print(f"{len(combos)} combinations", file=sys.stderr)
        return 0
    if args.prefix is None:
        parser.error("--prefix is required")
    args.prefix = args.prefix.resolve()
    args.out = args.out.resolve()
    args.out.mkdir(parents=True, exist_ok=True)

    lock = threading.Lock()
    done = 0
    rows = []
    with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
        futures = {pool.submit(run_combo, args, family, c): combo_name(c)
                   for c in combos}
        for future in concurrent.futures.as_completed(futures):
            result = future.result()
            with lock:
                done += 1
                rows += result
                statuses = sorted({r["status"] for r in result})
                print(f"[{done}/{len(combos)}] {futures[future]}: "
                      f"{', '.join(statuses)}", flush=True)

    rows.sort(key=lambda r: (r["combo"], r["variant"], r["backend"]))
    axis_names = list(FAMILY_RULES[family][0](args).keys())
    failed = write_summary(args, axis_names, rows)
    report_failures(args, failed)
    print(f"{len(rows)} runs, {len(failed)} failed; see {args.out}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
