"""Check fmpercfl note and vibrato state."""

import math
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest


class FmpercflTests(unittest.TestCase):
    def setUp(self):
        repo = Path(__file__).resolve().parents[2]
        self.executable = Path(os.environ.get("CSOUND_TEST_EXECUTABLE",
                                             repo / "build" / "csound")).resolve()
        if not self.executable.is_file():
            self.skipTest("Set CSOUND_TEST_EXECUTABLE to the built csound command")
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)

    def render(self, depth=.1, speed=6, block=16, offset=0, fullscale=1,
               table_size=128, score=None):
        source = self.root / "test.csd"
        wave = self.root / "output.wav"
        if score is None:
            score = f"i 1 {offset/32768} .125 220"
        source.write_text(f'''<CsoundSynthesizer>
<CsOptions>
-d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr=32768
ksmps={block}
nchnls=1
0dbfs={fullscale}
giSine ftgen 1,0,1024,10,1
giVibrato ftgen 2,0,{table_size},-7,0,{table_size//4},1,{table_size//2},-1,{table_size//4},0
instr 1
aOut fmpercfl .125*0dbfs,p4,.5,.5,{depth},{speed},giSine,giSine,giSine,giSine,giVibrato
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

    def test_note_reuse(self):
        for depth,speed in ((0,6),(.1,6),(.1,-6)):
            with self.subTest(depth=depth, speed=speed):
                actual = self.render(depth=depth, speed=speed, score=
                                     "i 1 0 .125 220\n"
                                     "i 1 .25 .125 440\n"
                                     "i 1 .5 .125 220")
                self.assertGreater(max(map(abs, actual)), .01)
                self.assert_signal(actual[:4096], actual[16384:20480])
                fresh = self.render(depth=depth, speed=speed, score="i 1 0 .125 440")
                self.assert_signal(actual[8192:12288], fresh)

    def test_vibrato_rate_and_table(self):
        reference = self.render()
        for size in (8,512):
            self.assert_signal(self.render(table_size=size), reference)
        self.assert_signal(self.render(speed=0), self.render(depth=0))
        self.assertGreater(max(abs(a-b) for a,b in
                               zip(reference,self.render(speed=12))), .01)

    def test_partial_blocks_and_scale(self):
        reference = self.render()
        actual = self.render(offset=3)
        self.assertEqual(actual[:3], (0.0,)*3)
        self.assertEqual(max(map(abs, actual[4099:]), default=0), 0)
        self.assert_signal(actual[3:4099], reference)
        self.assert_signal(self.render(block=1), reference)
        self.assert_signal(self.render(fullscale=32768), reference)


if __name__ == "__main__":
    unittest.main()
