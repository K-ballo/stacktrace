#!/usr/bin/env python3
# Eggs.Stacktrace
#
# Copyright (c) 2026 Agustin Berge
#
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt

"""Annotate any warning in a SARIF report that falls on a line changed
relative to a base ref (e.g. origin/main).
"""

import json
import os
import re
import subprocess
import sys

HUNK_RE = re.compile(r"^@@ -\d+(?:,\d+)? \+(?P<start>\d+)(?:,(?P<count>\d+))? @@")


def changed_lines(base_ref: str) -> dict[str, set[int]]:
    diff = subprocess.run(
        ["git", "diff", "--unified=0", f"{base_ref}...HEAD"],
        capture_output=True,
        text=True,
        check=True,
    ).stdout

    changed: dict[str, set[int]] = {}
    current_file = None
    for line in diff.splitlines():
        if line.startswith("+++ "):
            path = line[4:]
            current_file = None if path == "/dev/null" else path.removeprefix("b/")
        elif line.startswith("@@") and current_file is not None:
            match = HUNK_RE.match(line)
            if not match:
                continue
            start = int(match["start"])
            count = int(match["count"]) if match["count"] is not None else 1
            if count == 0:
                continue  # pure deletion, nothing added at this location
            changed.setdefault(current_file, set()).update(range(start, start + count))
    return changed


def main() -> int:
    if len(sys.argv) not in (3, 4):
        sys.exit(f"usage: {sys.argv[0]} <sarif-path> <base-ref> [label]")

    sarif_path, base_ref = sys.argv[1], sys.argv[2]
    label = sys.argv[3] if len(sys.argv) == 4 else "-Weverything"
    with open(sarif_path, encoding="utf-8") as f:
        results = json.load(f)["runs"][0]["results"]

    changed = changed_lines(base_ref)

    seen = set()
    flagged = []
    for result in results:
        location = result["locations"][0]["physicalLocation"]
        uri = location["artifactLocation"]["uri"]
        region = location["region"]
        if region["startLine"] not in changed.get(uri, set()):
            continue
        key = (uri, region["startLine"], result["ruleId"])
        if key in seen:
            continue
        seen.add(key)
        flagged.append((uri, region, result["ruleId"], result["message"]["text"]))

    if os.environ.get("GITHUB_ACTIONS") == "true":
        for uri, region, rule_id, message in flagged:
            print(
                f"::error file={uri},line={region['startLine']},col={region['startColumn']}::[{rule_id}] {message}"
            )

    if flagged:
        print(
            f"{len(flagged)} {label} warning(s) on lines changed relative to {base_ref}.",
            file=sys.stderr,
        )

    return 0


if __name__ == "__main__":
    sys.exit(main())
