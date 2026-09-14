"""Check tambourine resonance and note state."""

import math
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest


class TambourineTests(unittest.TestCase):
    def setUp(self):
        repo = Path(__file__).resolve().parents[2]
        self.executable = Path(os.environ.get("CSOUND_TEST_EXECUTABLE",
                                             repo / "build" / "csound")).resolve()
        if not self.executable.is_file():
            self.skipTest("Set CSOUND_TEST_EXECUTABLE to the built csound command")
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)

    def render(self, frequency=1000, block=16, offset=0, fullscale=1,
               dettack=.015625, settings="32,0,0", score=None):
        source = self.root / "test.csd"
        wave = self.root / "output.wav"
        if score is None:
            score = f"i 1 {offset/32768} .125 {frequency}"
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
aOut tambourine .125*0dbfs,{dettack},{settings},p4,5600,8100
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

    def test_repeated_resonance(self):
        for frequency in (1000, 2300, 0):
            for settings in ("32,0,0", "8,.5,.2"):
                with self.subTest(frequency=frequency, settings=settings):
                    actual = self.render(settings=settings, score=
                                         f"i 1 0 .125 {frequency}\n"
                                         f"i 1 .25 .125 {frequency}")
                    self.assertGreater(max(map(abs, actual[:4096])), .001)
                    self.assert_signal(actual[:4096], actual[8192:12288])

    def test_changed_resonance(self):
        actual = self.render(score="i 1 0 .125 1000\n"
                                   "i 1 .25 .125 2300\n"
                                   "i 1 .5 .125 0\n"
                                   "i 1 .75 .125 1000")
        self.assert_signal(actual[:4096], actual[24576:28672])
        self.assert_signal(actual[8192:12288], actual[16384:20480])
        self.assertGreater(max(abs(a-b) for a,b in
                               zip(actual[:4096], actual[8192:12288])), .001)

    def test_partial_blocks_and_scale(self):
        reference = self.render()
        actual = self.render(offset=3)
        self.assertEqual(actual[:3], (0.0,)*3)
        self.assertEqual(max(map(abs, actual[4099:]), default=0), 0)
        # The damping countdown runs at control rate. Compare before it fires.
        self.assert_signal(actual[3:2051], reference[:2048])
        self.assert_signal(self.render(block=1)[:2048], reference[:2048])
        self.assert_signal(self.render(fullscale=32768), reference)

    def test_damping_time(self):
        reference = self.render(dettack=0)
        damped = self.render(dettack=.0625)
        self.assert_signal(damped[:2032], reference[:2032])
        self.assertLess(sum(x*x for x in damped[3072:3584]),
                        .5*sum(x*x for x in reference[3072:3584]))


if __name__ == "__main__":
    unittest.main()
