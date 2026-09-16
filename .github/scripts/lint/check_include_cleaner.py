#!/usr/bin/env python3
# Eggs.Stacktrace
#
# Copyright (c) 2026 Agustin Berge
#
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt

# Fail if clang-include-cleaner finds a missing or unused #include.
#
# Runs in two passes: once per translation unit in the compilation database,
# and once per public header (using the VERIFY_INTERFACE_HEADER_SETS TUs).

from collections.abc import Callable
import json
import os
import pathlib
import shlex
import shutil
import subprocess
import sys

CLANG_INCLUDE_CLEANER = "clang-include-cleaner"

# Directory component suffixes CMake uses for the <target>_verify_interface_
# header_sets / <target>_verify_private_header_sets object libraries.
VERIFY_HEADER_SETS_DIRS = (
    "_verify_interface_header_sets",
    "_verify_private_header_sets",
)

EXCLUDED_DIRS = (
    "test/cmake-fetch_content/",
    "test/cmake-find_package/",
)

# Flags stripped from a compile_commands.json entry before reuse: the
# compiler executable and output/mode flags are build artifacts.
DROPPED_FLAGS = {"-c"}
DROPPED_FLAG_PAIRS = {"-o"}

Target = tuple[pathlib.Path, str, list[str]]


def parse_command(command: str, file: str) -> list[str]:
    tokens = shlex.split(command)
    args = []
    skip_next = False
    for i, token in enumerate(tokens):
        if skip_next:
            skip_next = False
            continue
        if i == 0 or token == file:
            continue
        if token in DROPPED_FLAG_PAIRS:
            skip_next = True
            continue
        if token in DROPPED_FLAGS:
            continue
        args.append(token)
    return args


def gh_escape_property(value: str) -> str:
    return (
        value.replace("%", "%25")
        .replace("\r", "%0D")
        .replace("\n", "%0A")
        .replace(":", "%3A")
        .replace(",", "%2C")
    )


def run_include_cleaner(args: list[str]) -> list[str]:
    result = subprocess.run(
        [CLANG_INCLUDE_CLEANER, "--print=changes", *args],
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        sys.exit(
            f"error: {CLANG_INCLUDE_CLEANER} failed on {args}:\n{result.stderr}"
        )
    return [line for line in result.stdout.splitlines() if line]


def tracked_files(repo_root: pathlib.Path) -> set[str]:
    out = subprocess.run(
        ["git", "ls-files", "-z"],
        cwd=repo_root,
        check=True,
        capture_output=True,
        text=True,
    ).stdout
    return {line for line in out.split("\0") if line}


def run_checks(
    compile_commands: list[dict], target: Callable[[dict], Target | None]
) -> tuple[dict[str, list[str]], set[str]]:
    findings = {}
    checked = set()
    for entry in compile_commands:
        resolved = target(entry)
        if resolved is None:
            continue
        file, rel, args = resolved
        if rel in checked:
            continue
        checked.add(rel)

        lines = run_include_cleaner([str(file), "--", *args])
        if lines:
            findings[rel] = lines
    return findings, checked


def translation_unit_target(
    entry: dict, repo_root: pathlib.Path, tracked: set[str]
) -> Target | None:
    file = pathlib.Path(entry["file"])
    if not file.is_absolute():
        file = pathlib.Path(entry["directory"]) / file
    try:
        rel = file.resolve().relative_to(repo_root).as_posix()
    except ValueError:
        return None
    if rel not in tracked:
        return None  # e.g. build-directory-generated sources
    if rel.startswith(EXCLUDED_DIRS) or rel.endswith(".compile_fail.cpp"):
        return None

    args = parse_command(entry["command"], entry["file"])
    return file, rel, args


def resolve_header(spelling: str, args: list[str]) -> pathlib.Path | None:
    for arg in args:
        if not arg.startswith("-I"):
            continue
        candidate = pathlib.Path(arg[2:]) / spelling
        if candidate.is_file():
            return candidate
    return None


def header_target(entry: dict, repo_root: pathlib.Path) -> Target | None:
    wrapper = pathlib.Path(entry["file"])
    parts = wrapper.parts
    idx = next(
        (
            i
            for i, part in enumerate(parts)
            if any(part.endswith(suffix) for suffix in VERIFY_HEADER_SETS_DIRS)
        ),
        None,
    )
    if idx is None:
        return None
    spelling = "/".join(parts[idx + 1 :])[: -len(".cxx")]

    args = parse_command(entry["command"], entry["file"])
    header = resolve_header(spelling, args)
    if header is None:
        sys.exit(f"error: could not resolve header {spelling!r} for {wrapper}")

    rel = header.resolve().relative_to(repo_root).as_posix()
    return header, rel, args


def main() -> int:
    if len(sys.argv) != 2:
        sys.exit(f"usage: {sys.argv[0]} <build-dir>")

    if shutil.which(CLANG_INCLUDE_CLEANER) is None:
        sys.exit(f"error: {CLANG_INCLUDE_CLEANER} not found on PATH")

    build_dir = pathlib.Path(sys.argv[1]).resolve()
    repo_root = pathlib.Path(
        subprocess.run(
            ["git", "rev-parse", "--show-toplevel"],
            check=True,
            capture_output=True,
            text=True,
        ).stdout.strip()
    )

    compile_commands = json.loads(
        (build_dir / "compile_commands.json").read_text(encoding="utf-8")
    )
    tracked = tracked_files(repo_root)

    tu_findings, tu_checked = run_checks(
        compile_commands,
        lambda entry: translation_unit_target(entry, repo_root, tracked),
    )
    header_findings, header_checked = run_checks(
        compile_commands,
        lambda entry: header_target(entry, repo_root),
    )

    findings = {**tu_findings, **header_findings}
    if findings:
        print("error: clang-include-cleaner findings:")
        in_ci = os.environ.get("GITHUB_ACTIONS") == "true"
        summary_lines = []
        for path in sorted(findings):
            print(f"  {path}")
            for line in findings[path]:
                print(f"    {line}")
            summary_lines.append(f"{path}")
            summary_lines.extend(f"  {line}" for line in findings[path])
            if in_ci:
                summary = "; ".join(findings[path])
                print(
                    f"::error file={gh_escape_property(path)}::"
                    f"clang-include-cleaner: {gh_escape_property(summary)}"
                )

        step_summary = os.environ.get("GITHUB_STEP_SUMMARY")
        if step_summary:
            with open(step_summary, "a", encoding="utf-8") as f:
                f.write("## clang-include-cleaner findings\n\n```\n")
                f.write("\n".join(summary_lines))
                f.write("\n```\n")
        return 1

    print(
        f"OK: no missing/unused includes across {len(tu_checked)} translation "
        f"unit(s) and {len(header_checked)} header(s)."
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
