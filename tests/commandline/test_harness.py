#!/usr/bin/env python3
"""Check test discovery, expectations, and subprocess handling without Csound."""

import ast
import importlib.util
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock

from test_metadata import TestCase, discover_tests, load_test

SPEC = importlib.util.spec_from_file_location("csound_commandline_tests",
                                             Path(__file__).with_name("test.py"))
HARNESS = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(HARNESS)


class MetadataTests(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)

    def write(self, name="test.csd", metadata=None, raw=None):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        header = "" if metadata is None else "<CsTest>\n" + metadata + "\n</CsTest>\n"
        path.write_text(raw if raw is not None else
                        header + "<CsoundSynthesizer>\n<CsInstruments>\n" +
                        "</CsInstruments>\n</CsoundSynthesizer>\n")
        return path

    def test_discovery_is_recursive_sorted_and_needs_no_registration(self):
        self.write("z.csd")
        self.write("arrays/a.csd", 'description = "array example"')
        self.write("ignore.txt")
        cases = discover_tests(self.root)
        self.assertEqual([case.filename for case in cases], ["arrays/a.csd", "z.csd"])
        self.assertEqual(cases[0].description, "array example")
        self.assertEqual(cases[1].expect, {"exit": 0})
        self.write("new.csd")
        self.assertEqual(len(discover_tests(self.root)), 3)

    def test_special_arguments_preserve_empty_strings_and_spaces(self):
        path = self.write(metadata='''
args = []
application_args = ["--", "first violin", ""]
stack_limit_kb = 256
''')
        case = load_test(path, self.root)
        self.assertEqual(case.args, [])
        self.assertEqual(case.application_args, ["--", "first violin", ""])
        self.assertEqual(case.stack_limit_kb, 256)

    def test_profile_expectation_and_skip_live_in_the_csd(self):
        path = self.write(metadata='''
[profiles.wasm.expect]
exit = "nonzero"
stderr = ["no threads"]
''')
        self.assertEqual(load_test(path, self.root).expect, {"exit": 0})
        self.assertEqual(load_test(path, self.root, "wasm").expect["exit"], "nonzero")
        path = self.write(metadata='profiles.wasm.skip = "Needs threads"')
        self.assertFalse(load_test(path, self.root).skip)
        self.assertEqual(load_test(path, self.root, "wasm").skip, "Needs threads")

    def test_invalid_metadata_fails_with_filename(self):
        for data in ('typo = 1', 'expect.exit = "nonzero"',
                     'expect.exit = true', 'expect.exit = -11',
                     'expect = {exit = 0, stderr = [""]}',
                     'expect = {exit = 0, stderr_regex = ["["]}',
                     'args = "-n"', 'stack_limit_kb = -1',
                     'profiles.wasm.skip = true'):
            with self.subTest(data=data):
                path = self.write(metadata=data)
                with self.assertRaisesRegex(ValueError, "test.csd:"):
                    load_test(path, self.root)

    def test_malformed_duplicate_and_unclosed_headers_are_errors(self):
        for source in ('<CsTest>\nexpect = [\n</CsTest>',
                       '<CsTest>\nexit = 0',
                       '</CsTest>',
                       '<CsTest>\n</CsTest>\n<CsTest>\n</CsTest>',
                       '<CsTest>\nskip="a"\nskip="b"\n</CsTest>',
                       '<CsTest>\n[expect]\n[expect]\n</CsTest>',
                       '<CsTest>\n{"expect": {"exit": 0}}\n</CsTest>'):
            with self.subTest(source=source):
                with self.assertRaises(ValueError):
                    load_test(self.write(raw=source), self.root)

    def test_toml_comments_literal_regex_and_multiline_arrays(self):
        path = self.write(metadata=r'''
# Literal strings keep regular-expression backslashes unchanged.
description = "check brackets"
[expect]
exit = "nonzero"
stderr_regex = [
    'Opcode\[\] failure',
]
''')
        case = load_test(path, self.root)
        self.assertEqual(case.description, "check brackets")
        self.assertEqual(case.expect["stderr_regex"], [r"Opcode\[\] failure"])

    def test_metadata_is_read_only_before_synthesizer_tag(self):
        for tag in ("CsoundSynthesizer", "CsoundSynthesiser"):
            with self.subTest(tag=tag):
                path = self.write(raw=f"<{tag}>\n<CsInstruments>\n"
                                      'Stext = "<CsTest>"\n'
                                      f"</CsInstruments>\n</{tag}>\n")
                self.assertEqual(load_test(path, self.root).expect, {"exit": 0})
                path = self.write(raw='<CsTest>\nskip="manual"\n</CsTest>\n'
                                      + path.read_text())
                self.assertEqual(load_test(path, self.root).skip, "manual")

    def test_empty_directory_is_not_a_passing_suite(self):
        with self.assertRaisesRegex(ValueError, "no .csd tests"):
            discover_tests(self.root)


class ResultTests(unittest.TestCase):
    def result(self, code=1, stderr="wanted diagnostic", stdout="", error=None,
               expect=None):
        case = TestCase("example.csd", "example", expect or {
            "exit": "nonzero", "stderr": ["wanted diagnostic"]})
        return HARNESS.TestResult(0, case, code, stderr + stdout, .01, error,
                                  stderr_output=stderr, stdout_output=stdout)

    def test_nonzero_exit_requires_the_right_diagnostic(self):
        self.assertTrue(self.result().passed)
        self.assertFalse(self.result(stderr="unrelated parser failure").passed)
        self.assertFalse(self.result(code=0).passed)
        self.assertIn("missing substring", self.result(stderr="wrong").get_formatted_output(1))

    def test_stdout_cannot_satisfy_stderr_expectations(self):
        self.assertFalse(self.result(stderr="", stdout="wanted diagnostic").passed)

    def test_all_substrings_and_regexes_must_match_on_the_right_stream(self):
        expect = {"exit": 0, "stderr": ["first", "second"],
                  "stdout_regex": [r"count=\d+"]}
        self.assertTrue(self.result(0, "first second", "count=42", expect=expect).passed)
        self.assertFalse(self.result(0, "first", "count=42", expect=expect).passed)
        self.assertFalse(self.result(0, "first second count=42", "", expect=expect).passed)

    def test_exact_exit_code(self):
        expect = {"exit": 2, "stderr": ["wanted"]}
        self.assertTrue(self.result(2, expect=expect).passed)
        self.assertFalse(self.result(1, expect=expect).passed)

    def test_crashes_and_harness_errors_never_pass(self):
        for code in (-11, -6, 0xC0000005, -1073741819, 0x80000003, 0x40000015):
            with self.subTest(code=code):
                self.assertFalse(self.result(code).passed)
        self.assertFalse(self.result(error="Test timed out").passed)
        self.assertFalse(self.result(error="Executable not found").passed)


class ProcessTests(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory(prefix="csound harness ")
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)
        for name, value in (("csoundExecutable", sys.executable),
                            ("runtimeExecutable", None), ("runtimeArguments", []),
                            ("sourceDirectory", str(self.root)), ("test_timeout", 5)):
            patcher = mock.patch.object(HARNESS, name, value)
            patcher.start()
            self.addCleanup(patcher.stop)

    def script(self, body):
        script = self.root / "fake csound.py"
        script.write_text(body)
        return script

    def test_actual_subprocess_captures_streams_and_arguments_separately(self):
        script = self.script('import sys\nprint(repr(sys.argv[2:]))\n'
                             'print("wanted diagnostic", file=sys.stderr)\nsys.exit(2)\n')
        case = TestCase("test file.csd", "argv", {"exit": 2,
                       "stderr": ["wanted diagnostic"],
                       "stdout": ["['--', 'first violin', '']"]},
                        args=[str(script)], application_args=["--", "first violin", ""])
        result = HARNESS.execute_single_test(0, case, ["-nd"], str(self.root))
        self.assertTrue(result.passed, result.get_formatted_output(1) + result.cs_output)
        self.assertNotIn("wanted diagnostic", result.stdout_output)

    def test_runtime_wrapper_preserves_module_path_and_arguments(self):
        script = self.script('import sys\nprint(repr(sys.argv[1:]))\n')
        for name in ("module with spaces.cwasm", r"module\with\backslashes.cwasm"):
            with self.subTest(name=name):
                module = self.root / name
                case = TestCase("nested/test.csd", "runtime")
                with mock.patch.object(HARNESS, "runtimeExecutable", sys.executable), \
                     mock.patch.object(HARNESS, "runtimeArguments", [str(script), "--runtime-option"]), \
                     mock.patch.object(HARNESS, "csoundExecutable", str(module)):
                    result = HARNESS.execute_single_test(0, case, [], str(self.root))
                self.assertTrue(result.passed, result.get_formatted_output(1) + result.cs_output)
                self.assertEqual(ast.literal_eval(result.stdout_output),
                                 ["--runtime-option", str(module), "-nd", "nested/test.csd"])

    def test_timeout_and_missing_executable_do_not_satisfy_negative_test(self):
        script = self.script('import time\ntime.sleep(5)\n')
        case = TestCase("test.csd", "timeout", {"exit": "nonzero", "stderr": ["wanted"]},
                        args=[str(script)])
        with mock.patch.object(HARNESS, "test_timeout", .05):
            result = HARNESS.execute_single_test(0, case, [])
        self.assertFalse(result.passed)
        self.assertIn("timed out", result.error)
        with mock.patch.object(HARNESS, "csoundExecutable", str(self.root / "missing")):
            result = HARNESS.execute_single_test(0, case, [])
        self.assertFalse(result.passed)
        self.assertIsNotNone(result.error)

    def test_fixture_copy_is_writable_and_does_not_change_sources(self):
        fixture = self.root / "fixture.txt"
        fixture.write_text("original")
        with HARNESS.runtime_working_directory() as cwd:
            Path(cwd, "fixture.txt").write_text("changed")
        self.assertEqual(fixture.read_text(), "original")

    def test_parallel_results_keep_discovery_order(self):
        script = self.script('import sys,time\ntime.sleep(float(sys.argv[-1]))\n')
        cases = [TestCase(f"{i}.csd", str(i), args=[str(script)],
                          application_args=[delay])
                 for i, delay in enumerate((".05", "0", ".01"))]
        results = HARNESS.run_tests_parallel(cases, [], max_workers=3)
        self.assertEqual([r.filename for r in results], ["0.csd", "1.csd", "2.csd"])
        self.assertTrue(all(r.passed for r in results))


if __name__ == "__main__":
    unittest.main()
