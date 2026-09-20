#!/usr/bin/env python3
# Eggs.Stacktrace
#
# Copyright (c) 2026 Agustin Berge
#
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt

# Fail if a tracked header/source file is not referenced by any CMake target.

import argparse
import json
import os
import pathlib
import subprocess
import sys

SOURCE_EXTENSIONS = {".h", ".hpp", ".c", ".cpp"}

EXCLUDED_DIRS = (
    "test/cmake-fetch_content/",
    "test/cmake-find_package/",
)


def gh_escape_property(value: str) -> str:
    return (
        value.replace("%", "%25")
        .replace("\r", "%0D")
        .replace("\n", "%0A")
        .replace(":", "%3A")
        .replace(",", "%2C")
    )


def tracked_source_files(repo_root: pathlib.Path) -> set[str]:
    out = subprocess.run(
        ["git", "ls-files", "-z"],
        cwd=repo_root,
        check=True,
        capture_output=True,
        text=True,
    ).stdout

    files = set()
    for line in out.split("\0"):
        if not line:
            continue
        if pathlib.PurePosixPath(line).suffix not in SOURCE_EXTENSIONS:
            continue
        if line.startswith(EXCLUDED_DIRS):
            continue
        files.add(line)
    return files


def referenced_source_files(build_dir: pathlib.Path) -> set[str]:
    reply_dir = build_dir / ".cmake" / "api" / "v1" / "reply"
    index_files = sorted(reply_dir.glob("index-*.json"))
    if not index_files:
        sys.exit(f"error: no CMake File API reply found in {reply_dir}")

    index = json.loads(index_files[-1].read_text(encoding="utf-8"))
    codemodel_file = reply_dir / index["reply"]["codemodel-v2"]["jsonFile"]
    codemodel = json.loads(codemodel_file.read_text(encoding="utf-8"))

    referenced = set()
    for target_ref in codemodel["configurations"][0]["targets"]:
        target = json.loads(
            (reply_dir / target_ref["jsonFile"]).read_text(encoding="utf-8")
        )
        for source in target.get("sources", []):
            if not source.get("isGenerated", False):
                referenced.add(source["path"])
    return referenced


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--dump-referenced", metavar="OUT_JSON")
    parser.add_argument("paths", nargs="+", metavar="BUILD_DIR|REFERENCED_JSON")
    args = parser.parse_args()

    if args.dump_referenced is not None:
        if len(args.paths) != 1:
            parser.error("--dump-referenced takes exactly one <build-dir>")
        referenced = referenced_source_files(pathlib.Path(args.paths[0]).resolve())
        pathlib.Path(args.dump_referenced).write_text(
            json.dumps(sorted(referenced), indent=2), encoding="utf-8"
        )
        print(f"OK: recorded {len(referenced)} referenced source files.")
        return 0

    repo_root = pathlib.Path(
        subprocess.run(
            ["git", "rev-parse", "--show-toplevel"],
            check=True,
            capture_output=True,
            text=True,
        ).stdout.strip()
    )

    tracked = tracked_source_files(repo_root)
    referenced: set[str] = set()
    for referenced_json in args.paths:
        referenced.update(
            json.loads(pathlib.Path(referenced_json).read_text(encoding="utf-8"))
        )

    orphans = sorted(tracked - referenced)
    if orphans:
        print("error: files not referenced by any CMake target:")
        in_ci = os.environ.get("GITHUB_ACTIONS") == "true"
        for path in orphans:
            print(f"  {path}")
            if in_ci:
                print(
                    f"::error file={gh_escape_property(path)}::"
                    "not referenced by any CMake target"
                )
        return 1

    print(
        f"OK: all {len(tracked)} tracked header/source files "
        "are referenced by a CMake target."
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
