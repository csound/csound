"""Discover CSD tests and read their in-file expectations."""

import re
from dataclasses import dataclass, field
from pathlib import Path

try:
    import tomllib
except ModuleNotFoundError:
    try:
        import tomli as tomllib
    except ModuleNotFoundError as error:
        raise SystemExit("Python before 3.11 needs tomli: install "
                         "tests/commandline/requirements.txt") from error


HEADER = re.compile(r"^[ \t]*<CsTest>[ \t]*\n(.*?)^[ \t]*</CsTest>[ \t]*$",
                    re.M | re.S)
SYNTHESIZER = re.compile(r"^[ \t]*<CsoundSynthesi[sz]er>", re.M)


@dataclass
class TestCase:
    filename: str
    description: str
    expect: dict = field(default_factory=lambda: {"exit": 0})
    args: list = field(default_factory=lambda: ["-nd"])
    application_args: list = field(default_factory=list)
    stack_limit_kb: int = None
    skip: str = ""


def _strings(value, name):
    if not isinstance(value, list) or not all(isinstance(x, str) for x in value):
        raise ValueError(f"{name} must be a list of strings")
    return value


def validate_expectation(expect):
    if not isinstance(expect, dict) or "exit" not in expect:
        raise ValueError("expect must be a table with an exit value")
    allowed = {"exit", "stderr", "stderr_regex", "stdout", "stdout_regex"}
    if set(expect) - allowed:
        raise ValueError(f"unknown expectation keys: {sorted(set(expect) - allowed)}")
    status = expect["exit"]
    if status != "nonzero" and not (type(status) is int and 0 <= status <= 255):
        raise ValueError('expect.exit must be 0..255 or "nonzero"')
    for stream in allowed - {"exit"}:
        patterns = _strings(expect.get(stream, []), f"expect.{stream}")
        if any(not pattern for pattern in patterns):
            raise ValueError(f"expect.{stream} cannot contain empty patterns")
        if stream.endswith("_regex"):
            for pattern in patterns:
                try:
                    re.compile(pattern)
                except re.error as error:
                    raise ValueError(f"invalid {stream}: {error}") from error
    if status != 0 and not (expect.get("stderr") or expect.get("stderr_regex")):
        raise ValueError("expected failures must declare a stderr diagnostic")


def load_test(path, root, profile="native"):
    filename = path.relative_to(root).as_posix()
    try:
        source = path.read_text(encoding="utf-8-sig")
        # Only the preamble belongs to the runner. Tag-like strings in the
        # orchestra or score are Csound input, not test metadata.
        start = SYNTHESIZER.search(source)
        preamble = source[:start.start()] if start else source
        headers = HEADER.findall(preamble)
        if (len(headers) > 1 or preamble.count("<CsTest>") != len(headers)
                or preamble.count("</CsTest>") != len(headers)):
            raise ValueError("expected one complete <CsTest> block before "
                             "<CsoundSynthesizer>")
        data = tomllib.loads(headers[0]) if headers else {}
        allowed = {"description", "expect", "args", "application_args",
                   "stack_limit_kb", "skip", "profiles"}
        if set(data) - allowed:
            raise ValueError(f"unknown metadata keys: {sorted(set(data) - allowed)}")
        profiles = data.pop("profiles", {})
        if not isinstance(profiles, dict) or set(profiles) - {"native", "wasm"}:
            raise ValueError("profiles must contain native or wasm overrides")
        for name, override in profiles.items():
            if not isinstance(override, dict) or set(override) - {"expect", "skip"}:
                raise ValueError(f"profile {name} may override expect or skip")
            if "expect" in override:
                validate_expectation(override["expect"])
            if "skip" in override and not isinstance(override["skip"], str):
                raise ValueError("skip must be a reason string")
        # Validate the default too, even if this profile overrides it.
        validate_expectation(data.get("expect", {"exit": 0}))
        data.update(profiles.get(profile, {}))
        for key in ("description", "skip"):
            if key in data and not isinstance(data[key], str):
                raise ValueError(f"{key} must be a string")
        for key in ("args", "application_args"):
            if key in data:
                _strings(data[key], key)
        limit = data.get("stack_limit_kb")
        if limit is not None and (type(limit) is not int or limit <= 0):
            raise ValueError("stack_limit_kb must be a positive integer")
        return TestCase(filename, data.pop("description", filename), **data)
    except (ValueError, OSError) as error:
        raise ValueError(f"{filename}: {error}") from error


def discover_tests(directory, profile="native"):
    root = Path(directory)
    paths = sorted(root.rglob("*.csd"), key=lambda p: p.relative_to(root).as_posix())
    if not paths:
        raise ValueError(f"no .csd tests found in {root}")
    return [load_test(path, root, profile) for path in paths]
