#!/usr/bin/env python3
# Eggs.Stacktrace
#
# Copyright (c) 2026 Agustin Berge
#
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt

"""Fail if a merged -Weverything SARIF report contains warnings from headers."""

import json
import os
import sys

HEADER_PREFIX = "include/"


def main() -> int:
    if len(sys.argv) != 2:
        sys.exit(f"usage: {sys.argv[0]} <sarif-path>")

    with open(sys.argv[1], encoding="utf-8") as f:
        results = json.load(f)["runs"][0]["results"]

    seen = set()
    flagged = []
    for result in results:
        location = result["locations"][0]["physicalLocation"]
        uri = location["artifactLocation"]["uri"]
        if not uri.startswith(HEADER_PREFIX):
            continue
        region = location["region"]
        key = (uri, region["startLine"], region["startColumn"], result["ruleId"])
        if key in seen:
            continue
        seen.add(key)
        flagged.append(result)

    lines = []
    in_ci = os.environ.get("GITHUB_ACTIONS") == "true"
    for result in flagged:
        location = result["locations"][0]["physicalLocation"]
        uri = location["artifactLocation"]["uri"]
        region = location["region"]
        message = f"[{result['ruleId']}] {result['message']['text']}"
        line = f"{uri}:{region['startLine']}:{region['startColumn']}: {message}"
        lines.append(line)
        print(line)
        if in_ci:
            print(
                f"::error file={uri},line={region['startLine']},"
                f"col={region['startColumn']}::{message}"
            )

    step_summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if flagged and step_summary:
        with open(step_summary, "a", encoding="utf-8") as f:
            f.write("## -Weverything: warnings under include/\n\n")
            f.write("```\n")
            f.write("\n".join(lines))
            f.write("\n```\n")

    if flagged:
        print(
            f"{len(flagged)} warning(s) from headers ",
            file=sys.stderr,
        )
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
