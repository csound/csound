"""Check fmb3 pitch updates and note state."""

import math
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest


class Fmb3Tests(unittest.TestCase):
    def setUp(self):
        repo = Path(__file__).resolve().parents[2]
        self.executable = Path(os.environ.get("CSOUND_TEST_EXECUTABLE",
                                             repo / "build" / "csound")).resolve()
        if not self.executable.is_file():
            self.skipTest("Set CSOUND_TEST_EXECUTABLE to the built csound command")
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)

    def render(self, depth=0, speed=6, block=16, offset=0, fullscale=1,
               score=None, switch=False, reference=False, change_pitch=False):
        source = self.root / "test.csd"
        wave = self.root / "output.wav"
        if score is None:
            score = f"i 1 {offset/32768} .125 220"
        vibrato = "-7,1,128,1" if switch or reference else "10,1"
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
giVibrato ftgen 2,0,128,{vibrato}
instr 1
kFrequency init p4
kDepth init {depth}
kCycle timeinstk
if {int(change_pitch)} == 1 && kCycle > 64 then
  kFrequency = 2*p4
endif
if {int(switch)} == 1 then
  kDepth = (kCycle <= 64 ? 10 : 0)
endif
if {int(reference)} == 1 then
  kFrequency = (kCycle <= 64 ? 2*p4 : p4)
endif
aOut fmb3 .125*0dbfs,kFrequency,.5,.5,kDepth,{speed},giSine,giSine,giSine,giSine,giVibrato
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

    def test_zero_vibrato_pitch(self):
        for change_pitch in (False, True):
            with self.subTest(change_pitch=change_pitch):
                actual = self.render(change_pitch=change_pitch)
                reference = self.render(depth=.1, speed=0,
                                        change_pitch=change_pitch)
                self.assertGreater(max(map(abs, actual)), .01)
                self.assert_signal(actual, reference)

    def test_disable_vibrato(self):
        # A constant vibrato table doubles the pitch until depth becomes zero.
        actual = self.render(switch=True, speed=0)
        reference = self.render(reference=True)
        self.assert_signal(actual, reference)

    def test_note_reuse(self):
        for depth in (0, .3):
            with self.subTest(depth=depth):
                actual = self.render(depth=depth, score="i 1 0 .125 220\n"
                                     "i 1 .25 .125 440\n"
                                     "i 1 .5 .125 220")
                self.assert_signal(actual[:4096], actual[16384:20480])
                fresh = self.render(depth=depth, score="i 1 0 .125 440")
                self.assert_signal(actual[8192:12288], fresh)

    def test_partial_blocks_and_scale(self):
        for depth in (0, .3):
            with self.subTest(depth=depth):
                reference = self.render(depth=depth)
                actual = self.render(depth=depth, offset=3)
                self.assertEqual(actual[:3], (0.0,)*3)
                self.assertEqual(max(map(abs, actual[4099:]), default=0), 0)
                self.assert_signal(actual[3:4099], reference)
                self.assert_signal(self.render(depth=depth, block=1), reference)
                self.assert_signal(self.render(depth=depth, fullscale=32768),
                                   reference)


if __name__ == "__main__":
    unittest.main()
