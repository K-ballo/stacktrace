#!/usr/bin/env python3
# Eggs.Stacktrace
#
# Copyright (c) 2026 Agustin Berge
#
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt

"""CMAKE_CXX_COMPILER_LAUNCHER wrapper that captures per-TU -Weverything
diagnostics as a SARIF result fragment, without altering the real build.
"""

import json
import os
import re
import subprocess
import sys

DIAGNOSTIC_RE = re.compile(
    r"^(?P<file>[^:\n]+):(?P<line>\d+):(?P<column>\d+): warning: (?P<message>.*)$"
)
FLAG_RE = re.compile(r"^(?P<text>.*) \[-(?P<flag>W[\w-]+)\]$")


def find_output(args: list[str]) -> str | None:
    for prev, arg in zip(args, args[1:]):
        if prev == "-o":
            return arg
    return None


def parse_diagnostics(stderr: str) -> list[dict]:
    source_dir = os.environ.get("EGGS_STACKTRACE_SOURCE_DIR")
    results = []
    for line in stderr.splitlines():
        match = DIAGNOSTIC_RE.match(line)
        if not match:
            continue

        file = match["file"]
        if source_dir:
            try:
                file = os.path.relpath(file, source_dir)
            except ValueError:
                pass

        flag_match = FLAG_RE.match(match["message"])
        rule_id = f"-{flag_match['flag']}" if flag_match else "clang-diagnostic"
        text = flag_match["text"] if flag_match else match["message"]

        results.append(
            {
                "ruleId": rule_id,
                "level": "warning",
                "message": {"text": text},
                "locations": [
                    {
                        "physicalLocation": {
                            "artifactLocation": {"uri": file.replace(os.sep, "/")},
                            "region": {
                                "startLine": int(match["line"]),
                                "startColumn": int(match["column"]),
                            },
                        },
                    }
                ],
            }
        )
    return results


def main() -> int:
    compiler, *args = sys.argv[1:]
    proc = subprocess.run([compiler, *args], capture_output=True, text=True)
    sys.stdout.write(proc.stdout)
    sys.stderr.write(proc.stderr)

    output = find_output(args)
    if output is not None:
        results = parse_diagnostics(proc.stderr)
        sidecar = output + ".sarif.json"
        os.makedirs(os.path.dirname(sidecar) or ".", exist_ok=True)
        with open(sidecar, "w", encoding="utf-8") as f:
            json.dump(results, f)

    return proc.returncode


if __name__ == "__main__":
    sys.exit(main())
