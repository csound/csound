"""Check that a stalled configure does not stop later PipeWire cases."""

import contextlib
import importlib.util
import io
from pathlib import Path
import subprocess
import unittest
from unittest.mock import patch


spec = importlib.util.spec_from_file_location(
    "pipewire_detection", Path(__file__).resolve().parents[1] / "pipewire_detection.py")
probe = importlib.util.module_from_spec(spec)
spec.loader.exec_module(probe)


class PipeWireDetectionTests(unittest.TestCase):
    def test_timeout_fails_case_and_continues(self):
        for output in (None, b"configure started\n\xff"):
            with self.subTest(output=output):
                results = [subprocess.TimeoutExpired("cmake", 60, output=output)]
                results += [subprocess.CompletedProcess("cmake", 0, "")] * 7
                captured = io.StringIO()
                with patch.object(probe.subprocess, "run", side_effect=results) as run, \
                        patch("sys.argv", ["pipewire_detection.py"]), \
                        contextlib.redirect_stdout(captured):
                    failed = probe.main()
                self.assertTrue(failed)
                self.assertEqual(run.call_count, 8)
                for call in run.call_args_list:
                    self.assertGreater(call.kwargs["timeout"], 0)
                    self.assertLessEqual(call.kwargs["timeout"], 60)
                text = captured.getvalue()
                self.assertIn("legacy: FAIL", text)
                self.assertIn("timed out", text)
                self.assertIn("stale-headers: PASS", text)
                self.assertEqual(text.count(": PASS"), 7)
                if output:
                    self.assertIn("configure started\n\ufffd", text)


if __name__ == "__main__":
    unittest.main()
