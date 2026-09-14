"""Check sleighbells note history and stopping state."""

import math
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest


class SleighbellsTests(unittest.TestCase):
    def setUp(self):
        repo = Path(__file__).resolve().parents[2]
        self.executable = Path(os.environ.get("CSOUND_TEST_EXECUTABLE",
                                             repo / "build" / "csound")).resolve()
        if not self.executable.is_file():
            self.skipTest("Set CSOUND_TEST_EXECUTABLE to the built csound command")
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)

    def render(self, block=16, offset=0, fullscale=1, dettack=0,
               damping=0, bells=32, score=None):
        source = self.root / "test.csd"
        wave = self.root / "output.wav"
        if score is None:
            score = f"i 1 {offset/32768} .125 .5 2500"
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
kAmp init p4*0dbfs
aOut sleighbells kAmp,{dettack},{bells},{damping},0,p5,5300,6500
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

    def test_silent_note_after_sounding_note(self):
        actual = self.render(score="i 1 0 .03125 .5 2500\n"
                                   "i 1 .0625 .03125 0 2500")
        self.assertGreater(max(map(abs, actual[:1024])), .01)
        self.assertEqual(max(map(abs, actual[2048:3072])), 0)

    def test_note_reuse(self):
        for damping, bells in ((0,32),(.02,64)):
            with self.subTest(damping=damping, bells=bells):
                actual = self.render(damping=damping, bells=bells, score=
                                     "i 1 0 .125 .5 2500\n"
                                     "i 1 .25 .125 .25 1000\n"
                                     "i 1 .5 .125 .5 2500")
                self.assert_signal(actual[:4096], actual[16384:20480])
                fresh = self.render(damping=damping, bells=bells,
                                    score="i 1 0 .125 .25 1000")
                self.assert_signal(actual[8192:12288], fresh)

    def test_stop_time(self):
        reference = self.render()
        stopped = self.render(dettack=.0625)
        # A fractional control cycle still truncates to the same stop time.
        self.assert_signal(self.render(dettack=.0625 + .25/2048), stopped)
        self.assert_signal(stopped[:2032], reference[:2032])
        self.assertGreater(max(abs(a-b) for a,b in
                               zip(stopped[2048:], reference[2048:])), .001)
        # A stop at or before the first block must prevent excitation.
        for dettack in (.125, .25):
            with self.subTest(dettack=dettack):
                self.assertEqual(max(map(abs, self.render(dettack=dettack))), 0)

    def test_partial_blocks_and_scale(self):
        reference = self.render()
        actual = self.render(offset=3)
        self.assertEqual(actual[:3], (0.0,)*3)
        self.assertEqual(max(map(abs, actual[4099:]), default=0), 0)
        # Compare before the control-rate stop at the end of the note.
        self.assert_signal(actual[3:2051], reference[:2048])
        self.assert_signal(self.render(block=1)[:2048], reference[:2048])
        self.assert_signal(self.render(fullscale=32768), reference)
        half = self.render(score="i 1 0 .125 .25 2500")
        self.assert_signal(half, tuple(.5*x for x in reference))


if __name__ == "__main__":
    unittest.main()
