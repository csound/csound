"""Check date/dates epoch conversion and host time range handling."""

from datetime import datetime, timedelta, timezone
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest


class DatesEpochTests(unittest.TestCase):
    def setUp(self):
        default = Path(__file__).resolve().parents[2] / "build" / "csound"
        self.executable = Path(os.environ.get("CSOUND_TEST_EXECUTABLE", default)).resolve()
        if not self.executable.is_file():
            self.skipTest("Set CSOUND_TEST_EXECUTABLE to the built csound command")

    def run_orchestra(self, body):
        csd = """<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
instr 1
""" + body + """
endin
</CsInstruments>
<CsScore>
i 1 0 .01
e
</CsScore>
</CsoundSynthesizer>
"""
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "test.csd"
            path.write_text(csd)
            result = subprocess.run([str(self.executable), str(path)],
                                    env={**os.environ, "TZ": "UTC"},
                                    capture_output=True, text=True, timeout=30)
        return result.returncode, result.stdout + result.stderr

    def test_epoch_and_current_time(self):
        body = """
  iNow date
  SNow dates
  SNegative dates -0.25
  SRoundtrip dates iNow
  SZero dates 0
  SOne dates 1
  SLower dates 0.25
  SUpper dates 0.75
  prints "CASE current|%s", SNow
  prints "CASE negative|%s", SNegative
  prints "CASE roundtrip|%s", SRoundtrip
  prints "CASE zero|%s", SZero
  prints "CASE one|%s", SOne
  prints "CASE lower|%s", SLower
  prints "CASE upper|%s", SUpper
"""
        # Hosts with a 64-bit time type must not narrow through int32_t.
        if sys.maxsize > 2**32:
            body += '  SFuture dates 2147483648\n  prints "CASE future|%s", SFuture\n'
        before = datetime.now(timezone.utc)
        status, output = self.run_orchestra(body)
        after = datetime.now(timezone.utc)
        self.assertEqual(status, 0, output)
        values = {key: datetime.strptime(value.strip(), "%a %b %d %H:%M:%S %Y")
                  .replace(tzinfo=timezone.utc)
                  for key, value in re.findall(r"CASE (\w+)\|([^\r\n]+)", output)}
        single = "(float samples)" in output
        epoch = datetime(2010 if single else 1970, 1, 1, tzinfo=timezone.utc)
        for key, seconds in (("zero", 0), ("one", 1), ("lower", 0), ("upper", 1)):
            self.assertEqual(values[key], epoch + timedelta(seconds=seconds), output)
        if sys.maxsize > 2**32:
            self.assertEqual(values["future"], epoch + timedelta(seconds=2**31), output)
        # Float timestamps lose some seconds of precision, but must retain the year.
        for key in ("current", "negative", "roundtrip"):
            tolerance = timedelta(seconds=64 if single else 2)
            self.assertGreaterEqual(values[key], before - tolerance, output)
            self.assertLessEqual(values[key], after + tolerance, output)

    def test_out_of_range_time(self):
        status, output = self.run_orchestra("  SDate dates 1e30\n")
        self.assertNotEqual(status, 0, output)
        self.assertIn("dates: time out of range", output)


if __name__ == "__main__":
    unittest.main()
