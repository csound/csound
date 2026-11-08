#!/usr/bin/env python3
"""Check file-local functions and read-only pointer parameters with Clang."""

import argparse
from collections import defaultdict
from concurrent.futures import ThreadPoolExecutor
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys

from clang import cindex


ROOT = Path(__file__).resolve().parents[1]
SOURCE_ROOTS = {"Engine", "Frontends", "InOut", "OOps", "Opcodes", "Top",
                "util", "util1", "platform"}
# Upstream libraries and generated code that live outside third_party/.
EXCLUDED_DIRS = ("InOut/libmpadec/", "util/SDIF/")
EXCLUDED_FILES = {"InOut/alphanumcmp.c", "OOps/pffft.c",
                  "Opcodes/tl/fractalnoise.cpp"}
EXCLUDED_COMPONENTS = {"third_party", "vendor", "external", "node_modules",
                       "vcpkg", "tests", "examples"}
SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".cxx"}
REFERENCE_SUFFIXES = SOURCE_SUFFIXES | {
    ".h", ".hpp", ".inc", ".def", ".lex", ".l", ".y", ".yy",
    ".js", ".i", ".m", ".mm", ".py", ".rs", ".cs",
}
IDENTIFIER = re.compile(r"\b[A-Za-z_]\w*\b")
IGNORE = re.compile(r"csound-linkage-ignore:\s*[A-Za-z0-9]")
REQUIRED_CHECKS = {"readability-non-const-parameter", "misc-use-internal-linkage"}


def checked_source(path):
    """Only first-party implementation files can produce diagnostics."""
    return (path.parts[0] in SOURCE_ROOTS
            and path.suffix in SOURCE_SUFFIXES
            and not EXCLUDED_COMPONENTS.intersection(path.parts)
            and path.as_posix() not in EXCLUDED_FILES
            and not path.as_posix().startswith(EXCLUDED_DIRS))


def tracked_files(root):
    result = subprocess.run(["git", "ls-files", "-z"], cwd=root, check=True,
                            capture_output=True, text=True)
    return [Path(name) for name in result.stdout.split("\0") if name]


def reference_files(root, paths):
    """Keep references from headers, tests and other platforms, too.

    This deliberately counts names in inactive code and strings. Missing a
    possible cleanup is safer than hiding a symbol another build or host uses.
    Clang, rather than this index, identifies the function definitions.
    """
    references = defaultdict(set)
    for path in paths:
        if path.suffix in REFERENCE_SUFFIXES:
            for name in set(IDENTIFIER.findall(
                    (root / path).read_text(errors="replace"))):
                references[name].add((root / path).resolve())
    return references


def compilation_entries(build, root, paths):
    selected = {(root / path).resolve() for path in paths
                if checked_source(path)}
    entries = {}
    for entry in json.loads((build / "compile_commands.json").read_text()):
        source = (Path(entry["directory"]) / entry["file"]).resolve()
        if source in selected:
            entries.setdefault(source, entry)
    if not entries:
        raise ValueError("No tracked first-party source files in compile_commands.json")
    return entries


def parse_arguments(entry, source, resource_dir, sdk):
    arguments = entry.get("arguments") or shlex.split(entry["command"])
    result = []
    skip = False
    for argument in arguments[1:]:
        if skip:
            skip = False
        elif argument in ("-o", "-MF", "-MT", "-MQ"):
            skip = True
        elif argument not in ("-c", "-MD", "-MMD", str(source), entry["file"]):
            result.append(argument)
    # libclang does not inherit the compiler driver's resource or SDK paths.
    result += ["-working-directory=" + entry["directory"],
               "-resource-dir=" + resource_dir]
    if sdk and "-isysroot" not in result:
        result += ["-isysroot", sdk]
    return result


def check_c_linkage(source, entry, references, resource_dir, sdk):
    """Check C linkage with references beyond the current translation unit.

    Use libclang's declarations to check C helpers, including local forward
    declarations. Leave shared interfaces and explicitly exported functions
    alone, even when this build does not use them from another file.
    """
    unit = cindex.Index.create().parse(
        str(source), args=parse_arguments(entry, source, resource_dir, sdk))
    errors = [str(d) for d in unit.diagnostics
              if d.severity >= cindex.Diagnostic.Error]
    if errors:
        return errors

    declarations = defaultdict(list)
    for cursor in unit.cursor.get_children():
        if (cursor.kind == cindex.CursorKind.FUNCTION_DECL
                and cursor.location.file
                and Path(cursor.location.file.name).resolve() == source):
            declarations[cursor.spelling].append(cursor)

    data = source.read_bytes()
    lines = data.decode(errors="replace").splitlines()
    candidates = {}
    for name, cursors in declarations.items():
        if name == "main" or references[name] - {source}:
            continue
        for cursor in cursors:
            if (not cursor.is_definition()
                    or cursor.linkage != cindex.LinkageKind.EXTERNAL
                    or cursor.storage_class == cindex.StorageClass.EXTERN):
                continue
            # Registration macros generate external loader entry points.
            offset = cursor.location.offset
            if (data[offset:offset + len(name)] != name.encode()
                    or not data[offset + len(name):].lstrip().startswith(b"(")):
                continue
            if any(child.kind in (cindex.CursorKind.VISIBILITY_ATTR,
                                  cindex.CursorKind.DLLEXPORT_ATTR)
                   for child in cursor.get_children()):
                continue
            start_line = cursor.extent.start.line
            previous = lines[start_line - 2] if start_line > 1 else ""
            if IGNORE.search(previous):
                continue
            candidates[name] = cursor

    if not candidates:
        return errors
    used = set()
    for top in unit.cursor.get_children():
        if not top.location.file or Path(top.location.file.name).resolve() != source:
            continue
        caller = top.spelling if top.kind == cindex.CursorKind.FUNCTION_DECL else None
        for cursor in top.walk_preorder():
            if cursor.kind == cindex.CursorKind.DECL_REF_EXPR:
                target = cursor.referenced
                if (target and target.kind == cindex.CursorKind.FUNCTION_DECL
                        and target.spelling != caller):
                    used.add(target.spelling)
    for name, cursor in candidates.items():
        # Unused functions belong to a separate dead-code review.
        if name in used:
            errors.append(
                f"{source}:{cursor.location.line}:{cursor.location.column}: "
                f"error: file-local function '{name}' should be static "
                "[csound-use-internal-linkage]")
    return errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("-p", "--build", type=Path, required=True)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--clang", default="clang")
    parser.add_argument("--clang-tidy", default="clang-tidy")
    parser.add_argument("--libclang", default=os.environ.get("LIBCLANG_LIBRARY_FILE"),
                        help="Path to libclang.so or libclang.dylib")
    parser.add_argument("-j", "--jobs", type=int, default=min(4, os.cpu_count() or 1))
    args = parser.parse_args()
    root, build = args.root.resolve(), args.build.resolve()
    if args.libclang:
        cindex.Config.set_library_file(args.libclang)
    available = subprocess.check_output(
        [args.clang_tidy, "--list-checks", "--config-file=" + str(ROOT / ".clang-tidy")],
        text=True)
    missing = REQUIRED_CHECKS - set(available.split())
    if missing:
        raise ValueError("clang-tidy lacks required checks: " + ", ".join(sorted(missing)))
    paths = tracked_files(root)
    references = reference_files(root, paths)
    entries = compilation_entries(build, root, paths)
    resource_dir = subprocess.check_output(
        [args.clang, "-print-resource-dir"], text=True).strip()
    sdk = (subprocess.check_output(["xcrun", "--show-sdk-path"], text=True).strip()
           if sys.platform == "darwin" else "")

    def check(item):
        source, entry = item
        command = [args.clang_tidy, str(source), "-p", str(build),
                   "--config-file=" + str(ROOT / ".clang-tidy"), "--quiet"]
        if source.suffix == ".c":
            # Our C check also protects references outside this translation unit.
            command.append("--checks=-misc-use-internal-linkage")
        result = subprocess.run(command,
            cwd=entry["directory"], capture_output=True, text=True)
        errors = ([result.stdout + result.stderr] if result.returncode else [])
        if source.suffix == ".c":
            errors += check_c_linkage(source, entry, references, resource_dir, sdk)
        return errors

    failures = []
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        for errors in pool.map(check, entries.items()):
            failures.extend(errors)
    if failures:
        print("\n".join(failures), file=sys.stderr)
        return 1
    print(f"Static and const checks passed for {len(entries)} source files")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, ValueError, subprocess.CalledProcessError,
            cindex.TranslationUnitLoadError, cindex.LibclangError) as error:
        sys.exit(str(error))
