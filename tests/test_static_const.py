"""Exercise the lint command on small C/C++ projects, including exclusions."""

import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


RUNNER = Path(__file__).resolve().parents[1] / "scripts/check_static_const.py"
CLANG = os.environ.get("CLANG", "clang")
CLANG_TIDY = os.environ.get("CLANG_TIDY", "clang-tidy")


class StaticConstTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="csound lint ")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name).resolve()
        self.entries = []
        subprocess.run(["git", "init", "-q", str(self.root)], check=True)
        self.add_file("H/api.h", "int api(void);\n")

    def add_file(self, name, source, compile=False):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(source)
        if compile:
            standard = "c11" if path.suffix == ".c" else "c++11"
            self.entries.append({
                "directory": str(self.root), "file": str(path),
                "arguments": [CLANG, "-std=" + standard, "-I", str(self.root),
                              "-c", str(path), "-o", str(path) + ".o"],
            })

    def check(self, should_pass):
        subprocess.run(["git", "add", "."], cwd=self.root, check=True)
        (self.root / "compile_commands.json").write_text(json.dumps(self.entries))
        result = subprocess.run(
            [sys.executable, str(RUNNER), "--root", str(self.root),
             "-p", str(self.root), "--clang", CLANG, "--clang-tidy", CLANG_TIDY],
            capture_output=True, text=True)
        output = result.stdout + result.stderr
        self.assertEqual(result.returncode == 0, should_pass, output)
        return output

    def test_missing_static_and_const_fail(self):
        self.add_file("Engine/sample.c", '''
#include "H/api.h"
int read_value(int *value) { return *value; }
int api(void) { int value = 7; return read_value(&value); }
''', compile=True)
        output = self.check(False)
        self.assertIn("csound-use-internal-linkage", output)
        self.assertIn("readability-non-const-parameter", output)

    def test_local_forward_declaration_does_not_hide_missing_static(self):
        self.add_file("Engine/sample.c", '''
#include "H/api.h"
int helper(void);
int api(void) { return helper(); }
int helper(void) { return 7; }
''', compile=True)
        self.assertIn("file-local function 'helper'", self.check(False))

    def test_function_used_only_in_registration_table_is_checked(self):
        self.add_file("Engine/sample.c", '''
#include "H/api.h"
int helper(void) { return 7; }
static int (*handlers[])(void) = {helper};
int api(void) { return handlers[0](); }
''', compile=True)
        self.assertIn("file-local function 'helper'", self.check(False))

    def test_writable_input_and_callback_are_allowed(self):
        self.add_file("Engine/sample.c", '''
#include "H/api.h"
static void write_value(int *value) { *value = 7; }
/* The callback signature accepts a writable pointer. */
/* NOLINTNEXTLINE(readability-non-const-parameter) */
static int read_callback(int *value) { return *value; }
int api(void) {
    int value = 0;
    write_value(&value);
    int (*callback)(int *) = read_callback;
    return callback(&value);
}
''', compile=True)
        self.check(True)

    def test_shared_and_exported_functions_are_allowed(self):
        self.add_file("H/shared.h", "int shared(void);\n")
        self.add_file("Engine/sample.c", '''
#include "H/api.h"
#include "H/shared.h"
int shared(void) { return 7; }
__attribute__((visibility("default"))) int exported(void) { return 3; }
#define REGISTER(name) int name(void) { return 1; }
REGISTER(module_entry)
int api(void) { return shared() + exported() + module_entry(); }
''', compile=True)
        self.check(True)

    def test_other_platform_and_test_references_are_kept(self):
        self.add_file("Engine/sample.c", '''
#include "H/api.h"
int platform_helper(void) { return 7; }
int test_helper(void) { return 3; }
int api(void) { return platform_helper() + test_helper(); }
''', compile=True)
        self.add_file("platform/windows/host.c", "int host(void) { return platform_helper(); }")
        self.add_file("tests/c/test.c", "int test(void) { return test_helper(); }")
        self.check(True)

    def test_excluded_files_are_not_parsed(self):
        self.add_file("Engine/sample.c", '#include "H/api.h"\nint api(void) { return 7; }',
                      compile=True)
        for name in ("third_party/library.c", "InOut/libmpadec/decoder.c",
                     "util/SDIF/reader.c", "OOps/pffft.c",
                     "Opcodes/tl/fractalnoise.cpp", "tests/c/test.c",
                     "InOut/alphanumcmp.c", "platform/vendor/library.c",
                     "platform/wasm/tests/test.c",
                     "build/generated.c"):
            self.add_file(name, '#include "does_not_exist.h"\n', compile=True)
        self.check(True)

    def test_documented_linkage_exception(self):
        self.add_file("Engine/sample.c", '''
#include "H/api.h"
// csound-linkage-ignore: A host looks up this symbol by name.
int special_entry(void) { return 7; }
int api(void) { return special_entry(); }
''', compile=True)
        self.check(True)

    def test_cpp_local_helper_is_checked(self):
        self.add_file("Engine/sample.cpp", '''
#include "H/api.h"
int helper() { return 7; }
int api() { return helper(); }
''', compile=True)
        self.assertIn("misc-use-internal-linkage", self.check(False))

    def test_linkage_exception_needs_a_reason(self):
        self.add_file("Engine/sample.c", '''
#include "H/api.h"
/* csound-linkage-ignore: */
int helper(void) { return 7; }
int api(void) { return helper(); }
''', compile=True)
        self.assertIn("csound-use-internal-linkage", self.check(False))

    def test_parse_errors_fail(self):
        self.add_file("Engine/sample.c", '#include "missing.h"\n', compile=True)
        self.check(False)

    def test_unused_debug_functions_are_left_for_dead_code_review(self):
        self.add_file("Engine/sample.c", '''
#include "H/api.h"
// helper() is useful when debugging.
const char *helper(void) { return "helper"; }
int recursive(int n) { return n ? recursive(n - 1) : 0; }
int disabled(void) { return 1; }
#if 0
int debug(void) { return disabled(); }
#endif
int api(void) { return 7; }
''', compile=True)
        self.check(True)

    def test_empty_scan_fails(self):
        self.add_file("third_party/library.c", "int helper(void) { return 7; }",
                      compile=True)
        self.assertIn("No tracked first-party", self.check(False))


if __name__ == "__main__":
    unittest.main()
