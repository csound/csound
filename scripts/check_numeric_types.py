#!/usr/bin/env python3
"""Check tracked C/C++ sources for legacy or unintended fixed-precision types."""

import argparse
from collections import defaultdict
import json
from pathlib import Path
import re
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]
EXCLUSIONS = "scripts/numeric_type_exclusions.json"
SOURCE_SUFFIXES = {
    ".c", ".h", ".cc", ".hh", ".cpp", ".hpp", ".cxx", ".hxx",
    ".m", ".mm", ".inc", ".i", ".l", ".lex", ".y",
}
REPLACEMENTS = {"MYFLT": "cs_float", "double": "cs_double"}
MARKER = "csound-numeric-ignore"

# Order matters. In particular, comments inside literals are not comments, and
# the apostrophe in a C++ digit separator must not start a character literal.
TOKENS = re.compile(
    r'(?P<comment>/\*.*?\*/|//[^\n]*)'
    r'|(?P<literal>(?:u8|u|U|L)?R"(?P<delimiter>[^\s()\\]{0,16})'
    r'\(.*?\)(?P=delimiter)"'
    r'|(?:u8|u|U|L)?"(?:\\.|[^"\\])*"'
    r"|(?:u8|u|U|L)?'(?:\\.|[^'\\])*')"
    r"|(?P<number>(?:\d|\.\d)(?:[eEpP][+-]|[\w.'])*)"
    r'|(?P<identifier>[A-Za-z_]\w*)|(?P<other>\S)',
    re.DOTALL,
)
IGNORE = re.compile(r"csound-numeric-ignore\s+([A-Za-z_, ]+)\s*:\s*(\S.*)")


def is_source(path):
    """Include header templates, bindings and parser/lexer inputs as well."""
    if path.endswith(".in"):
        path = path[:-3]
    return Path(path).suffix.lower() in SOURCE_SUFFIXES


def logical_lines(source):
    """C joins backslash-newline pairs before recognizing tokens or comments."""
    lines, physical_lines = [], []
    pending = ""
    first = 1
    for number, line in enumerate(re.findall(r"[^\n]*\n|[^\n]+$", source), 1):
        if line.endswith("\\\n"):
            pending += line[:-2]
        else:
            lines.append(pending + line)
            physical_lines.append(first)
            pending = ""
            first = number + 1
    if pending:
        lines.append(pending)
        physical_lines.append(first)
    return lines, physical_lines


def check_source(path, source):
    lines, physical_lines = logical_lines(source.replace("\r\n", "\n"))
    code = defaultdict(list)
    ignores = {}
    errors = []
    text = "".join(lines)
    line_number = 0
    previous_end = 0

    def report(line, message):
        errors.append(f"{path}:{physical_lines[line]}: {message}")

    for token in TOKENS.finditer(text):
        line_number += text.count("\n", previous_end, token.start())
        value = token.group()
        if token.lastgroup == "comment":
            if MARKER in value:
                body = value[2:-2] if value.startswith("/*") else value[2:]
                match = IGNORE.fullmatch(body.strip())
                if (not match or "\n" in value
                        or lines[line_number].strip() != value):
                    report(line_number, f"use a standalone {MARKER} comment "
                           "with type names and a reason above the code")
                else:
                    names = {name.strip() for name in match[1].split(",")}
                    if not names <= REPLACEMENTS.keys():
                        report(line_number, "ignore names must be double or MYFLT")
                    else:
                        ignores[line_number + 1] = names
        elif token.lastgroup != "literal":
            code[line_number].append(value)
        else:
            # Keep a placeholder so a literal before '#' cannot form a directive.
            code[line_number].append('""')
        line_number += value.count("\n")
        previous_end = token.end()

    for line in sorted(code.keys() | ignores.keys()):
        tokens = code[line]
        # Header names and diagnostic text are not C type declarations.
        if tokens[:1] == ["#"] and tokens[1:2] in (
                ["include"], ["include_next"], ["error"], ["warning"]):
            tokens = []
        forbidden = set(tokens) & REPLACEMENTS.keys()
        allowed = ignores.get(line, set())
        for name in sorted(allowed - forbidden):
            report(line - 1, f"unused {MARKER} for {name}; "
                   "the next logical line does not use it")
        for name in sorted(forbidden - allowed):
            report(line, f"use {REPLACEMENTS[name]} instead of {name}; "
                   f"if required, add /* {MARKER} {name}: reason */ above this line")
    return errors


def source_files(root):
    result = subprocess.run(
        ["git", "ls-files", "-z", "--cached"], cwd=root,
        check=True, capture_output=True, text=True,
    )
    return sorted({path for path in result.stdout.split("\0") if is_source(path)})


def excluded_files(root, files):
    """Only whole paths or directory prefixes, each with an explicit reason."""
    entries = json.loads((root / EXCLUSIONS).read_text(encoding="utf-8"))
    if not isinstance(entries, dict):
        raise ValueError(f"{EXCLUSIONS} must map paths to reasons")
    excluded = set()
    for path, reason in entries.items():
        if (not path or path.startswith("/") or "\\" in path
                or any(part in (".", "..", "") for part in path.rstrip("/").split("/"))
                or not isinstance(reason, str) or not reason.strip()):
            raise ValueError(f"{EXCLUSIONS}: invalid path or missing reason for {path!r}")
        if path.endswith("/"):
            matches = {name for name in files if name.startswith(path)}
        else:
            matches = {path} & set(files)
        if not matches:
            raise ValueError(f"{EXCLUSIONS}: unused exclusion {path!r}")
        excluded.update(matches)
    return excluded


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT,
                        help="repository to check (defaults to this checkout)")
    args = parser.parse_args(argv)
    try:
        files = source_files(args.root)
        excluded = excluded_files(args.root, files)
        errors = []
        for path in files:
            if path not in excluded:
                # Some old copyright comments use a single-byte encoding.
                source = (args.root / path).read_text(
                    encoding="utf-8", errors="surrogateescape")
                errors.extend(check_source(path, source))
    except (OSError, UnicodeError, ValueError, subprocess.CalledProcessError) as error:
        print(f"Numeric type check failed: {error}", file=sys.stderr)
        return 1
    if errors:
        print("\n".join(errors), file=sys.stderr)
        print("See doc/numeric-types.md for the policy and exceptions.", file=sys.stderr)
        return 1
    print(f"Numeric type check passed ({len(files) - len(excluded)} files checked, "
          f"{len(excluded)} excluded).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
