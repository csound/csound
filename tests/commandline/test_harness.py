#!/usr/bin/env python3

import importlib.util
import re
import unittest
from pathlib import Path
from unittest import mock


HARNESS_PATH = Path(__file__).with_name("test.py")
SPEC = importlib.util.spec_from_file_location("csound_commandline_tests", HARNESS_PATH)
HARNESS = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(HARNESS)


class TestResultTests(unittest.TestCase):
    def result(self, expected, return_code, stderr, error=None, output=None):
        return HARNESS.TestResult(
            0,
            ["failure.csd", "expected failure", expected],
            return_code,
            stderr if output is None else output,
            0.01,
            error=error,
            stderr_output=stderr,
        )

    def test_expected_failure_matches_stderr_substring(self):
        self.assertTrue(self.result("wanted diagnostic", 1, "wanted diagnostic").passed)

    def test_unrelated_failure_does_not_pass(self):
        result = self.result("wanted diagnostic", 1, "unrelated parse error")

        self.assertFalse(result.passed)
        self.assertIn(
            "Expected diagnostic was not found", result.get_formatted_output(1)
        )

    def test_expected_failure_matches_stderr_regular_expression(self):
        expected = re.compile(r"managed array (input|output) elements")

        self.assertTrue(
            self.result(expected, 1, "managed array input elements").passed
        )

    def test_diagnostic_is_checked_only_in_stderr(self):
        result = self.result(
            "wanted diagnostic",
            1,
            "other stderr",
            output="captured stdout: wanted diagnostic",
        )

        self.assertFalse(result.passed)

    def test_timeout_cannot_pass_as_an_expected_failure(self):
        result = self.result(1, -1, "", error="Test timed out")

        self.assertFalse(result.passed)

    def test_signal_cannot_pass_as_an_expected_failure(self):
        result = self.result(1, -6, "assertion failed")

        self.assertFalse(result.passed)

    def test_windows_exception_cannot_pass_as_an_expected_failure(self):
        result = self.result(1, 0xC0000005, "access violation")

        with mock.patch.object(HARNESS.os, "name", "nt"):
            self.assertFalse(result.passed)

    def test_empty_diagnostic_does_not_match_every_failure(self):
        result = self.result("", 1, "unrelated parse error")

        self.assertFalse(result.passed)
        self.assertIsNone(result.expected_stderr)


if __name__ == "__main__":
    unittest.main()
