#!/usr/bin/env python3
# Eggs.Stacktrace
#
# Copyright (c) 2026 Agustin Berge
#
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt

"""Select the integration jobs, and the backends within each, to run.

A change to a backend's own source (src/stacktrace_<backend>.cpp) reruns just
that backend on the toolchains that have it; any other change that can affect
a trace reruns everything; anything else runs nothing. Without a base commit
(manual runs, new branches, force pushes) everything runs.

Writes `matrix` (for strategy.matrix) and `any` to $GITHUB_OUTPUT.
"""

import argparse
import fnmatch
import json
import os
import subprocess
import sys

# `backends` lists what each toolchain can run, besides null.
JOBS = [
    {"os": "ubuntu-24.04", "compiler": "gcc-16", "toolchain": "gcc",
     "preset": "dev-gcc", "cc": "gcc-16", "cxx": "g++-16",
     "packages": "gcc-16 g++-16", "ppa": "ubuntu-toolchain-r",
     "backends": ["execinfo", "libbacktrace"]},
    {"os": "ubuntu-24.04", "compiler": "clang-22", "toolchain": "clang",
     "preset": "dev-clang", "cc": "clang-22", "cxx": "clang++-22",
     "packages": "clang-22 g++-16", "llvm_ver": "22",
     "ppa": "ubuntu-toolchain-r", "gcc_toolchain": "16",
     "backends": ["execinfo", "libbacktrace"]},
    {"os": "windows-2025-vs2026", "compiler": "msvc-2026",
     "toolchain": "msvc", "preset": "dev-msvc", "cxx": "cl",
     "backends": ["win32"]},
    {"os": "windows-2025-vs2026", "compiler": "clang-cl-20",
     "toolchain": "clang-cl", "preset": "dev-clang-cl", "cxx": "clang-cl",
     "backends": ["win32"]},
    {"os": "macos-15", "compiler": "apple-clang-16",
     "toolchain": "apple-clang", "preset": "dev-clang", "cxx": "clang++",
     "backends": ["execinfo"]},
]

# Keep in sync with the push `paths` filter in integration.yml.
AFFECTS_ALL = [
    "include/*",
    "src/*",
    "cmake/*",
    "CMakeLists.txt",
    "CMakePresets.json",
    "test/integration/*",
    ".github/workflows/integration.yml",
]


def classify(files):
    """Returns (all, backends): whether everything must run, else which
    backends changed."""
    backends = set()
    for f in files:
        if fnmatch.fnmatch(f, "src/stacktrace_*.cpp"):
            backends.add(f[len("src/stacktrace_"):-len(".cpp")])
        elif any(fnmatch.fnmatch(f, p) for p in AFFECTS_ALL):
            return True, set()
    return False, backends


def select(run_all, changed):
    jobs = []
    for job in JOBS:
        if run_all:
            run_backends = ""  # run_matrix.py runs all
        else:
            run_backends = " ".join(
                b for b in job["backends"] + ["null"] if b in changed
            )
            if not run_backends:
                continue
        jobs.append(dict(job, run_backends=run_backends))
    return jobs


def changed_files(base, head):
    """Files changed between `base` and `head`, or None without a base."""
    if not base or set(base) == {"0"}:
        return None
    exists = subprocess.run(
        ["git", "cat-file", "-e", f"{base}^{{commit}}"], capture_output=True
    )
    if exists.returncode != 0:
        return None
    out = subprocess.run(
        ["git", "diff", "--name-only", base, head],
        check=True, capture_output=True, text=True,
    ).stdout
    return [line for line in out.splitlines() if line]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--base", default="",
                        help="base commit; empty runs everything")
    parser.add_argument("--head", default="HEAD")
    args = parser.parse_args()

    files = changed_files(args.base, args.head)
    if files is None:
        run_all, changed = True, set()
    else:
        run_all, changed = classify(files)
    jobs = select(run_all, changed)

    print(f"all={run_all} changed={sorted(changed)}", file=sys.stderr)
    for job in jobs:
        print(f"  {job['compiler']}: {job['run_backends'] or 'all'}",
              file=sys.stderr)

    output = os.environ.get("GITHUB_OUTPUT")
    if output:
        with open(output, "a", encoding="utf-8") as f:
            f.write(f"matrix={json.dumps({'include': jobs})}\n")
            f.write(f"any={'true' if jobs else 'false'}\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
