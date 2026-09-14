"""Check fmvoice shared state and modulation."""

import math
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest


class FmvoiceTests(unittest.TestCase):
    def setUp(self):
        repo = Path(__file__).resolve().parents[2]
        self.executable = Path(os.environ.get("CSOUND_TEST_EXECUTABLE",
                                             repo / "build" / "csound")).resolve()
        if not self.executable.is_file():
            self.skipTest("Set CSOUND_TEST_EXECUTABLE to the built csound command")
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)

    def render(self, depth=.1, speed=6, vowel=4, tilt=50, block=16, offset=0,
               fullscale=1, sizes=(1024,)*4, triangle=False, score=None):
        source = self.root / "test.csd"
        wave = self.root / "output.wav"
        if score is None:
            score = f"i 1 {offset/32768} .125 220 {vowel} {tilt}"
        tables = []
        for number, size in enumerate(sizes, 1):
            shape = f"-7,0,{size//4},1,{size//2},-1,{size//4},0" if triangle else "10,1"
            tables.append(f"giWave{number} ftgen {number},0,{size},{shape}")
        table_source = "\n".join(tables)
        source.write_text(f'''<CsoundSynthesizer>
<CsOptions>
-d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr=32768
ksmps={block}
nchnls=1
0dbfs={fullscale}
{table_source}
giVibrato ftgen 5,0,128,10,1
instr 1
aOut fmvoice .125*0dbfs,p4,p5,p6,{depth},{speed},giWave1,giWave2,giWave3,giWave4,giVibrato
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
        for depth in (0, .1):
            with self.subTest(depth=depth):
                actual = self.render(depth=depth, score="i 1 0 .125 220 4 50\n"
                                     "i 1 .25 .125 440 32 90\n"
                                     "i 1 .5 .125 220 4 50")
                self.assertGreater(max(map(abs, actual)), .001)
                self.assert_signal(actual[:4096], actual[16384:20480])
                fresh = self.render(depth=depth, score="i 1 0 .125 440 32 90")
                self.assert_signal(actual[8192:12288], fresh)

    def test_vibrato_controls(self):
        reference = self.render(depth=0)
        self.assert_signal(self.render(speed=0), reference)
        moving = self.render(speed=6)
        self.assertGreater(max(abs(a-b) for a,b in zip(moving, reference)), .001)
        faster = self.render(speed=12)
        self.assertGreater(max(abs(a-b) for a,b in zip(moving, faster)), .001)

    def test_operator_table_lengths(self):
        reference = self.render(triangle=True)
        self.assert_signal(self.render(triangle=True, sizes=(128,256,512,2048)),
                           reference)

    def test_partial_blocks_and_scale(self):
        reference = self.render()
        actual = self.render(offset=3)
        self.assertEqual(actual[:3], (0.0,)*3)
        self.assertEqual(max(map(abs, actual[4099:]), default=0), 0)
        self.assert_signal(actual[3:4099], reference)
        self.assert_signal(self.render(block=1), reference)
        self.assert_signal(self.render(fullscale=32768), reference)

    def test_control_bounds(self):
        self.assert_signal(self.render(vowel=-1), self.render(vowel=0))
        self.assert_signal(self.render(vowel=128), self.render(vowel=127))
        self.assert_signal(self.render(tilt=-1), self.render(tilt=0))
        self.assert_signal(self.render(tilt=128), self.render(tilt=127))

    def test_vowel_banks(self):
        reference = self.render(vowel=0)
        for vowel in (31,32,64,96,127):
            with self.subTest(vowel=vowel):
                actual = self.render(vowel=vowel)
                self.assertGreater(max(map(abs, actual)), .001)
                self.assertGreater(max(abs(a-b) for a,b in zip(actual, reference)),
                                   .0001)


if __name__ == "__main__":
    unittest.main()
