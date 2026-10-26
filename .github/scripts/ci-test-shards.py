#!/usr/bin/env python3
"""Split native CI tests and check the results against the full test inventory."""

import argparse
from collections import Counter
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import xml.etree.ElementTree as ET


SOURCE = Path(__file__).resolve().parents[2]


def require_same(actual, expected, label):
    missing = Counter(expected) - Counter(actual)
    extra = Counter(actual) - Counter(expected)
    if missing or extra:
        raise ValueError(f"{label}: missing {dict(missing)}, extra {dict(extra)}")


def inventory(build):
    result = subprocess.run(
        ["ctest", "--test-dir", str(build), "--show-only=json-v1"],
        check=True, capture_output=True, text=True,
    )
    ctest = json.loads(result.stdout)["tests"]
    # Use the harness's discovery and native skip rules, including new tests.
    sys.path.insert(0, str(SOURCE / "tests/commandline"))
    from test_metadata import discover_tests
    csd = [test.filename for test in discover_tests(SOURCE / "tests/commandline")
           if not test.skip]
    return ctest, csd


def partition(ctest, csd, count):
    if count < 1:
        raise ValueError("Shard count must be positive")
    names = [test["name"] for test in ctest]
    for suite in (names, csd):
        if len(set(suite)) != len(suite):
            raise ValueError("Duplicate test names")
    # Keep fixture setup, consumers and cleanup on the same runner.
    groups = [{name} for name in names]
    fixtures = {}
    for test in ctest:
        for prop in test.get("properties", []):
            if prop["name"] in ("FIXTURES_SETUP", "FIXTURES_REQUIRED", "FIXTURES_CLEANUP"):
                for fixture in prop["value"]:
                    fixtures.setdefault(fixture, set()).add(test["name"])
    for members in fixtures.values():
        linked = [group for group in groups if group & members]
        groups = [group for group in groups if not group & members]
        groups.append(set().union(*linked))
    shards = [{"ctest": [], "csd": []} for _ in range(count)]
    for group in sorted(groups, key=lambda group: (-len(group), sorted(group))):
        target = min(shards, key=lambda shard: len(shard["ctest"]))
        target["ctest"].extend(sorted(group))
    for index, name in enumerate(sorted(csd)):
        shards[index % count]["csd"].append(name)
    for shard in shards:
        if not shard["ctest"] or not shard["csd"]:
            raise ValueError("Each shard must contain both CTest and CSD tests")
    plan = {"ctest": sorted(names), "csd": sorted(csd), "shards": shards}
    validate_plan(plan)
    return plan


def validate_plan(plan):
    if not plan["shards"]:
        raise ValueError("No test shards")
    for suite in ("ctest", "csd"):
        expected = plan[suite]
        if not expected or len(set(expected)) != len(expected):
            raise ValueError(f"Invalid {suite} inventory")
        assigned = [name for shard in plan["shards"] for name in shard[suite]]
        require_same(assigned, expected, f"{suite} assignments")


def write_json(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2) + "\n")


def make_plan(args):
    ctest, csd = inventory(args.build)
    plan = partition(ctest, csd, args.count)
    # CTest files and linked libraries can hold absolute build paths. Fail
    # explicitly if another runner uses a different workspace or build path.
    plan["source"] = str(SOURCE)
    plan["build"] = str(args.build.resolve())
    plan["commit"] = subprocess.check_output(
        ["git", "rev-parse", "HEAD"], cwd=SOURCE, text=True).strip()
    write_json(args.plan, plan)


def read_plan(path):
    plan = json.loads(path.read_text())
    validate_plan(plan)
    return plan


def run_shard(args):
    plan = read_plan(args.plan)
    if not 0 <= args.shard < len(plan["shards"]):
        raise ValueError("Shard index is out of range")
    if plan["source"] != str(SOURCE) or plan["build"] != str(args.build.resolve()):
        raise ValueError("Build and test runners must use the same workspace paths")
    commit = subprocess.check_output(
        ["git", "rev-parse", "HEAD"], cwd=SOURCE, text=True).strip()
    if commit != plan["commit"]:
        raise ValueError("Build and test runners checked out different commits")
    ctest, csd = inventory(args.build)
    require_same([test["name"] for test in ctest], plan["ctest"], "CTest inventory")
    require_same(csd, plan["csd"], "CSD inventory")
    selected = plan["shards"][args.shard]
    results = args.results.resolve()
    results.mkdir(parents=True, exist_ok=True)
    # The index-file interface also works with the CTest shipped by Ubuntu.
    # Resolve indices from this runner's inventory, after checking its names.
    indices = {test["name"]: index for index, test in enumerate(ctest, start=1)}
    selection = results / "ctest-indices.txt"
    selection.write_text("0,0,1," + ",".join(
        str(indices[name]) for name in selected["ctest"]) + "\n")
    junit = results / "ctest.xml"
    junit.unlink(missing_ok=True)
    test_status = subprocess.run([
        "ctest", "--test-dir", str(args.build),
        "--tests-information", str(selection), "--no-tests=error",
        "--output-on-failure", "--parallel", str(os.cpu_count() or 1),
        "--output-junit", str(junit),
    ]).returncode
    # Run the existing CSD harness with the same executable, plugin directory,
    # timeout, worker count and expected-output checks as the csdtests target.
    csd_dir = args.build.resolve() / "tests/commandline"
    report = csd_dir / "results.txt"
    report.unlink(missing_ok=True)
    csd_status = subprocess.run([
        sys.executable, str(SOURCE / "tests/commandline/test.py"),
        f"--csound-executable={args.build.resolve() / 'csound'}",
        f"--opcode7dir64={args.build.resolve()}",
        f"--source-dir={SOURCE / 'tests/commandline'}",
        *[f"--test={name}" for name in selected["csd"]],
    ], cwd=csd_dir).returncode
    output = report.read_text() if report.exists() else ""
    (results / "csd-results.txt").write_text(output)
    if args.reject_sanitizers and re.search(
            r"AddressSanitizer|UndefinedBehaviorSanitizer|runtime error:", output):
        csd_status = 1
    actual = {
        "shard": args.shard, "commit": commit,
        "ctest": [test.attrib["name"] for test in ET.parse(junit).iter("testcase")],
        "csd": re.findall(r"^\[(?:pass|FAIL)\] Test \d+: .* \((.+)\)$", output, re.M),
    }
    write_json(results / "executed.json", actual)
    for suite in ("ctest", "csd"):
        require_same(actual[suite], selected[suite], f"Executed {suite} tests")
    return int(bool(test_status or csd_status))


def audit(args):
    plan = read_plan(args.plan)
    reports = [json.loads(path.read_text())
               for path in args.results.rglob("executed.json")]
    require_same([report["shard"] for report in reports],
                 range(len(plan["shards"])), "Completed shards")
    for report in reports:
        if report["commit"] != plan["commit"]:
            raise ValueError("Test report is from another commit")
        for suite in ("ctest", "csd"):
            require_same(report[suite], plan["shards"][report["shard"]][suite],
                         f"Shard {report['shard']} {suite}")
    for suite in ("ctest", "csd"):
        require_same([name for report in reports for name in report[suite]],
                     plan[suite], f"Complete {suite} suite")
    print("Every planned test ran on exactly one shard.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    plan = commands.add_parser("plan")
    plan.add_argument("--build", required=True, type=Path)
    plan.add_argument("--plan", required=True, type=Path)
    plan.add_argument("--count", type=int, default=4)
    plan.set_defaults(function=make_plan)
    run = commands.add_parser("run")
    run.add_argument("--build", required=True, type=Path)
    run.add_argument("--plan", required=True, type=Path)
    run.add_argument("--shard", required=True, type=int)
    run.add_argument("--results", required=True, type=Path)
    run.add_argument("--reject-sanitizers", action="store_true")
    run.set_defaults(function=run_shard)
    check = commands.add_parser("audit")
    check.add_argument("--plan", required=True, type=Path)
    check.add_argument("--results", required=True, type=Path)
    check.set_defaults(function=audit)
    args = parser.parse_args()
    try:
        return args.function(args) or 0
    except (ValueError, OSError, subprocess.CalledProcessError, ET.ParseError) as error:
        print(error, file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
