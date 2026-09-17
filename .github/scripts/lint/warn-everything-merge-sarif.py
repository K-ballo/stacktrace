#!/usr/bin/env python3
# Eggs.Stacktrace
#
# Copyright (c) 2026 Agustin Berge
#
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt

"""Merge per-TU SARIF result fragments (written by warn-everything-launcher.py)
into a single conforming SARIF 2.1.0 log for the whole build.
"""

import json
import os
import pathlib
import sys


def main() -> int:
    if len(sys.argv) != 3:
        sys.exit(f"usage: {sys.argv[0]} <build-dir> <output-sarif>")

    build_dir = pathlib.Path(sys.argv[1])
    output = pathlib.Path(sys.argv[2])

    objects = sorted(build_dir.rglob("*.o"))
    fragments = sorted(build_dir.rglob("*.sarif.json"))

    on_actions = os.environ.get("GITHUB_ACTIONS") == "true"
    for obj in objects:
        sidecar = obj.with_name(obj.name + ".sarif.json")
        if not sidecar.exists():
            message = (
                f"no diagnostics captured for {obj} "
                "(compiler launcher not invoked for this translation unit?)"
            )
            print(f"::warning::{message}" if on_actions else message, file=sys.stderr)

    results = []
    for fragment in fragments:
        results.extend(json.loads(fragment.read_text(encoding="utf-8")))

    rules = [
        {"id": rule_id, "name": rule_id}
        for rule_id in sorted({r["ruleId"] for r in results})
    ]

    sarif = {
        "$schema": "https://docs.oasis-open.org/sarif/sarif/v2.1.0/cos02/schemas/sarif-schema-2.1.0.json",
        "version": "2.1.0",
        "runs": [
            {
                "tool": {
                    "driver": {
                        "name": "clang",
                        "informationUri": "https://clang.llvm.org/docs/UsersManual.html",
                        "rules": rules,
                    }
                },
                "results": results,
            }
        ],
    }
    output.write_text(json.dumps(sarif, indent=2), encoding="utf-8")

    counts: dict[str, int] = {}
    for result in results:
        counts[result["ruleId"]] = counts.get(result["ruleId"], 0) + 1
    by_rule = sorted(counts.items(), key=lambda kv: -kv[1])
    rule_lines = [f"{count:4d}  {rule_id}" for rule_id, count in by_rule]
    for line in rule_lines:
        print(line)
    total = f"{len(results)} warning(s) across {len(fragments)} translation unit(s) -> {output}"
    print(total)

    step_summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if results and step_summary:
        with open(step_summary, "a", encoding="utf-8") as f:
            f.write("## -Weverything summary\n\n```\n")
            f.write("\n".join([*rule_lines, total]))
            f.write("\n```\n")

    return 0


if __name__ == "__main__":
    sys.exit(main())
