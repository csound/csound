"""Check guiro amplitude, tooth counts, and stopping state."""

import math
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest


class GuiroTests(unittest.TestCase):
    def setUp(self):
        repo = Path(__file__).resolve().parents[2]
        self.executable = Path(os.environ.get("CSOUND_TEST_EXECUTABLE",
                                             repo / "build" / "csound")).resolve()
        if not self.executable.is_file():
            self.skipTest("Set CSOUND_TEST_EXECUTABLE to the built csound command")
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)

    def render(self, teeth=128, amplitude=.125, block=16, offset=0,
               fullscale=1, dettack=0, score=None):
        source = self.root / "test.csd"
        wave = self.root / "output.wav"
        if score is None:
            score = f"i 1 {offset/32768} .125 2500"
        source.write_text(f'''<CsoundSynthesizer>
<CsOptions>
-d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr=32768
ksmps={block}
nchnls=1
0dbfs={fullscale}
instr 1
seed 123
aOut guiro {amplitude}*0dbfs,{dettack},{teeth},0,0,p4,4000
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

    def test_amplitude_scale(self):
        reference = self.render()
        self.assertGreater(max(map(abs, reference)), .01)
        for fullscale in (2, 32768):
            with self.subTest(fullscale=fullscale):
                self.assert_signal(self.render(fullscale=fullscale), reference)
        self.assert_signal(self.render(amplitude=.25),
                           tuple(2*x for x in reference))
        self.assertEqual(max(map(abs, self.render(amplitude=0))), 0)

    def test_default_teeth(self):
        self.assert_signal(self.render(teeth=0), self.render(teeth=128))

    def test_fractional_teeth(self):
        lower = self.render(teeth=128.25)
        upper = self.render(teeth=128.75)
        # Both counts select the same random collisions. Their gain differs.
        ratio = (math.log(128.25)/128.25) / (math.log(128.75)/128.75)
        self.assert_signal(lower, tuple(ratio*x for x in upper))
        self.assertGreater(max(abs(a-b) for a,b in
                               zip(lower, self.render(teeth=128))), .001)

    def test_note_reuse(self):
        actual = self.render(score="i 1 0 .125 2500\n"
                                   "i 1 .25 .125 1000\n"
                                   "i 1 .5 .125 2500")
        self.assert_signal(actual[:4096], actual[16384:20480])
        fresh = self.render(score="i 1 0 .125 1000")
        self.assert_signal(actual[8192:12288], fresh)

    def test_partial_blocks(self):
        reference = self.render()
        actual = self.render(offset=3)
        self.assertEqual(actual[:3], (0.0,)*3)
        self.assertEqual(max(map(abs, actual[4099:]), default=0), 0)
        # Compare before the control-rate stop at the end of the note.
        self.assert_signal(actual[3:2051], reference[:2048])
        self.assert_signal(self.render(block=1)[:2048], reference[:2048])

    def test_stop_time(self):
        reference = self.render()
        stopped = self.render(dettack=.0625)
        self.assert_signal(stopped[:2032], reference[:2032])
        self.assertEqual(max(map(abs, stopped[2032:])), 0)
        for dettack in (.125, .25):
            with self.subTest(dettack=dettack):
                self.assertEqual(max(map(abs, self.render(dettack=dettack))), 0)


if __name__ == "__main__":
    unittest.main()
