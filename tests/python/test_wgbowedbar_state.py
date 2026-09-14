"""Check bowed-bar frequency limits and note state."""

import math
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest


class BowedBarTests(unittest.TestCase):
    def setUp(self):
        repo = Path(__file__).resolve().parents[2]
        self.executable = Path(os.environ.get("CSOUND_TEST_EXECUTABLE",
                                             repo / "build" / "csound")).resolve()
        if not self.executable.is_file():
            self.skipTest("Set CSOUND_TEST_EXECUTABLE to the built csound command")
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)

    def render(self, frequency=220, lowest=220, block=16, offset=0,
               fullscale=1, score=None):
        source = self.root / "test.csd"
        wave = self.root / "output.wav"
        if score is None:
            score = f"i 1 {offset/8192} .125 {frequency}"
        source.write_text(f'''<CsoundSynthesizer>
<CsOptions>
-d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr=8192
ksmps={block}
nchnls=1
0dbfs={fullscale}
instr 1
kFrequency init p4
if p4 == -1 then
  kCycle timeinstk
  kFrequency = 2000 + 100*(kCycle % 2)
endif
aOut wgbowedbar .125*0dbfs,kFrequency,.3,.5,.809,0,0,.2,{lowest}
out aOut
endin
</CsInstruments>
<CsScore>
{score}
e
</CsScore>
</CsoundSynthesizer>
''')
        run = subprocess.run([str(self.executable), "-W", "-f", "-o",
                              str(wave), str(source)], capture_output=True,
                             text=True, cwd=self.root, timeout=30)
        self.assertEqual(run.returncode, 0, run.stdout + run.stderr)
        raw = wave.read_bytes()
        position = 12
        while position + 8 <= len(raw):
            tag, size = struct.unpack_from("<4sI", raw, position)
            if tag == b"data":
                samples = struct.unpack_from(f"<{size//4}f", raw, position+8)
                self.assertTrue(all(map(math.isfinite, samples)))
                return samples
            position += 8 + size + (size & 1)
        self.fail("No WAV samples")

    def assert_signal(self, actual, expected):
        self.assertEqual(len(actual), len(expected))
        self.assertLess(max(abs(a-b) for a, b in zip(actual, expected)), 2e-6)

    def test_capped_pitch(self):
        reference = self.render(frequency=1568)
        self.assertGreater(max(map(abs, reference)), .001)
        for frequency in (1568, 2000, -1):
            with self.subTest(frequency=frequency):
                self.assert_signal(self.render(frequency=frequency), reference)

    def test_default_delay_capacity(self):
        reference = self.render(frequency=1568, lowest=0)
        for lowest in (0, 1568, 2000):
            with self.subTest(lowest=lowest):
                self.assert_signal(self.render(frequency=2000, lowest=lowest),
                                   reference)

    def test_note_reuse(self):
        for lowest in (0, 220):
            with self.subTest(lowest=lowest):
                actual = self.render(lowest=lowest, score=
                                     "i 1 0 .125 220\n"
                                     "i 1 .25 .125 2000\n"
                                     "i 1 .5 .125 220")
                self.assert_signal(actual[:1024], actual[4096:5120])
                reference = self.render(frequency=2000, lowest=lowest)
                self.assert_signal(actual[2048:3072], reference[:1024])

    def test_reused_delay_capacity(self):
        actual = self.render(lowest="p5", score="i 1 0 .125 220 110\n"
                             "i 1 .25 .125 2000 -1\n"
                             "i 1 .5 .125 220 -1")
        self.assert_signal(actual[:1024], actual[4096:5120])
        reference = self.render(frequency=2000, lowest=110)
        self.assert_signal(actual[2048:3072], reference[:1024])

    def test_sample_blocks_and_scale(self):
        for frequency in (220, 2000):
            with self.subTest(frequency=frequency):
                reference = self.render(frequency=frequency)
                actual = self.render(frequency=frequency, offset=3)
                self.assertEqual(actual[:3], (0.0,)*3)
                self.assertEqual(max(map(abs, actual[1027:]), default=0), 0)
                self.assert_signal(actual[3:1027], reference[:1024])
                self.assert_signal(self.render(frequency=frequency, block=1),
                                   reference)
                self.assert_signal(self.render(frequency=frequency,
                                               fullscale=32768), reference)


if __name__ == "__main__":
    unittest.main()
