"""Exercise the source policy without a Csound build or third-party modules."""

import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


SCRIPT = Path(__file__).resolve().parents[2] / "scripts/check_numeric_types.py"
spec = importlib.util.spec_from_file_location("numeric_type_policy", SCRIPT)
policy = importlib.util.module_from_spec(spec)
spec.loader.exec_module(policy)


class SourcePolicyTests(unittest.TestCase):
    def check(self, source):
        return policy.check_source("example.cpp", source)

    def test_flags_types_casts_macros_and_inactive_platform_code(self):
        errors = self.check(
            "MYFLT sample;\n"
            "#define CONVERT(x) ((double)(x))\n"
            "#if defined(OTHER_PLATFORM)\n"
            "long double value;\n"
            "#endif\n")
        self.assertEqual(len(errors), 3)
        self.assertIn("example.cpp:1: use cs_float", errors[0])
        self.assertIn("example.cpp:2: use cs_double", errors[1])
        self.assertIn("example.cpp:4: use cs_double", errors[2])

    def test_allows_aliases_fixed_float_and_longer_identifiers(self):
        self.assertEqual(self.check(
            "cs_float sample; cs_double sum; float wire_value;\n"
            "int double_buffer, MYFLT2LONG; csoundGetSizeOfMYFLT();\n"), [])

    def test_ignores_comments_literals_and_header_names(self):
        self.assertEqual(self.check(r'''
/* double old_value;
   MYFLT old_sample; */
// double unused;
const char *s = "double /* MYFLT */ \" //";
auto raw = u8R"csd(double ignored;
MYFLT ignored_too; " // /*
)csd";
auto wide = L"MYFLT double";
char quote = '\'';
#include <double/MYFLT.h>
#warning double is required here
#error MYFLT is deprecated
'''), [])

    def test_digit_separator_does_not_hide_later_declarations(self):
        errors = self.check("int n = 1'000; double x;\nMYFLT y;\n")
        self.assertEqual(len(errors), 2)
        self.assertIn("example.cpp:1:", errors[0])
        self.assertIn("example.cpp:2:", errors[1])

    def test_line_splices_follow_c_rules_and_keep_physical_locations(self):
        errors = self.check("// hidden \\\n double x;\n"
                            "dou\\\nble y;\nMYFLT z;\n")
        self.assertEqual(len(errors), 2)
        self.assertIn("example.cpp:3:", errors[0])
        self.assertIn("example.cpp:5:", errors[1])

    def test_explanation_allows_only_named_type_on_next_line(self):
        errors = self.check(
            "/* csound-numeric-ignore double: The library takes double. */\n"
            "double external; MYFLT sample;\n"
            "double other;\n")
        self.assertEqual(len(errors), 2)
        self.assertIn("example.cpp:2: use cs_float", errors[0])
        self.assertIn("example.cpp:3: use cs_double", errors[1])

    def test_allows_cpp_comment_and_continued_macro(self):
        self.assertEqual(self.check(
            "// csound-numeric-ignore double: Keep the library argument type.\n"
            "#define EXTERNAL(x) \\\n    ((double)(x))\n"), [])

    def test_compatibility_typedef_can_name_both_types(self):
        self.assertEqual(self.check(
            "/* csound-numeric-ignore double, MYFLT: Define a legacy alias. */\n"
            "typedef double MYFLT;\n"), [])

    def test_multiline_comment_preserves_later_error_locations(self):
        errors = self.check("/* Ignore this old code\n double old;\n */\nMYFLT x;\n")
        self.assertEqual(len(errors), 1)
        self.assertIn("example.cpp:4: use cs_float", errors[0])

    def test_stale_ignores_fail_including_at_end_of_file(self):
        for following in ("cs_double x;\n", "\ndouble x;\n",
                          '// "double"\n', "#include <double.h>\n", ""):
            with self.subTest(following=following):
                errors = self.check(
                    "/* csound-numeric-ignore double: External type. */\n" + following)
                self.assertIn("example.cpp:1: unused", errors[0])

    def test_rejects_missing_reason_unknown_type_and_trailing_comment(self):
        for comment in (
            "/* csound-numeric-ignore double: */",
            "/* csound-numeric-ignore float: External type. */",
            "/* csound-numeric-ignore double External type. */",
            "int x; /* csound-numeric-ignore double: External type. */",
        ):
            with self.subTest(comment=comment):
                errors = self.check(comment + "\ndouble x;\n")
                self.assertEqual(len(errors), 2)

    def test_literal_cannot_grant_an_exception(self):
        errors = self.check(
            'const char *s = "csound-numeric-ignore double: External type.";\n'
            'double x;\n')
        self.assertEqual(len(errors), 1)
        self.assertIn("example.cpp:2: use cs_double", errors[0])

    def test_multiple_adjacent_ignores_do_not_cover_each_other(self):
        errors = self.check(
            "/* csound-numeric-ignore MYFLT: Compatibility alias. */\n"
            "/* csound-numeric-ignore double: External format. */\n"
            "double x;\n")
        self.assertEqual(len(errors), 1)
        self.assertIn("example.cpp:1: unused", errors[0])

    def test_crlf_and_old_comment_encodings(self):
        self.assertEqual(self.check(
            "/* Copyright \udce4 */\r\n"
            "/* csound-numeric-ignore double: External type. */\r\n"
            "double x;\r\n"), [])


class CommandLineTests(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory(prefix="numeric policy ")
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)
        subprocess.run(["git", "init", "-q", str(self.root)], check=True)
        self.write(policy.EXCLUSIONS, "{}")

    def write(self, path, content, tracked=True):
        target = self.root / path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(content, encoding="utf-8")
        if tracked:
            subprocess.run(["git", "add", "--", path], cwd=self.root, check=True)

    def run_check(self):
        return subprocess.run([sys.executable, str(SCRIPT), "--root", str(self.root)],
                              capture_output=True, text=True)

    def test_scans_entire_tree_and_templates_but_not_untracked_builds(self):
        self.write("Engine/good.c", "cs_float sample; cs_double sum;\n")
        self.write("build/generated.c", "double x;", tracked=False)
        self.write("README.md", "MYFLT and double are deprecated here.")
        for path in ("new directory/type.h.in", "Engine/parser.y", "Java/wrapper.i"):
            self.write(path, "double x;\n")
        result = self.run_check()
        self.assertEqual(result.returncode, 1)
        self.assertIn("new directory/type.h.in:1:", result.stderr)
        self.assertIn("Engine/parser.y:1:", result.stderr)
        self.assertIn("Java/wrapper.i:1:", result.stderr)
        self.assertNotIn("build/generated.c", result.stderr)
        self.assertNotIn("README.md", result.stderr)

    def test_file_and_directory_exclusions_do_not_hide_nearby_code(self):
        self.write(policy.EXCLUSIONS, json.dumps({
            "vendor/": "Unmodified upstream source.",
            "external.h": "Header supplied by an external library.",
        }))
        self.write("vendor/math.c", "double x; MYFLT y;\n")
        self.write("external.h", "double external;\n")
        self.write("vendor_extra/math.c", "double x;\n")
        result = self.run_check()
        self.assertEqual(result.returncode, 1)
        self.assertIn("vendor_extra/math.c:1:", result.stderr)
        self.assertNotIn("vendor/math.c:", result.stderr)
        self.assertNotIn("external.h:", result.stderr)
        self.write("vendor_extra/math.c", "cs_double x;\n")
        result = self.run_check()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("1 files checked, 2 excluded", result.stdout)

    def test_bad_or_unused_exclusions_fail(self):
        for exclusions in ({"missing/": "No longer present."}, {"source.c": ""},
                           {"../": "Outside the tree."}, ["source.c"]):
            with self.subTest(exclusions=exclusions):
                self.write("source.c", "cs_double x;\n")
                self.write(policy.EXCLUSIONS, json.dumps(exclusions))
                result = self.run_check()
                self.assertEqual(result.returncode, 1)
                self.assertIn("numeric_type_exclusions.json", result.stderr)


if __name__ == "__main__":
    unittest.main()
