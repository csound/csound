"""Check paired FM oscillator feedback and note state."""

import math
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest


class FmPairTests(unittest.TestCase):
    def setUp(self):
        repo = Path(__file__).resolve().parents[2]
        self.executable = Path(os.environ.get("CSOUND_TEST_EXECUTABLE",
                                             repo / "build" / "csound")).resolve()
        if not self.executable.is_file():
            self.skipTest("Set CSOUND_TEST_EXECUTABLE to the built csound command")
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)

    def render(self, opcode, depth=0, block=16, offset=0, fullscale=1,
               first_size=1024, index=.5, crossfade=.5, score=None):
        source = self.root / "test.csd"
        wave = self.root / "output.wav"
        if score is None:
            score = f"i 1 {offset/32768} .125 256"
        source.write_text(f'''<CsoundSynthesizer>
<CsOptions>
-d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr=32768
ksmps={block}
nchnls=1
0dbfs={fullscale}
giFirst ftgen 1,0,{first_size},10,1
giSine ftgen 2,0,1024,10,1
giVibrato ftgen 3,0,128,10,1
instr 1
aOut {opcode} .125*0dbfs,p4,{index},{crossfade},{depth},6,giFirst,giSine,giSine,giSine,giVibrato
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

    def test_feedback_table(self):
        for opcode in ("fmbell", "fmrhode", "fmwurlie"):
            with self.subTest(opcode=opcode):
                # Crossfade=2 mutes the first carrier. Its table size must
                # therefore have no effect on the second pair's feedback.
                reference = self.render(opcode, crossfade=2)
                self.assertGreater(max(map(abs, reference)), .001)
                self.assert_signal(self.render(opcode, crossfade=2,
                                               first_size=2048), reference)

    def test_rhodes_carrier_gain(self):
        actual = self.render("fmrhode", index=0, crossfade=0)
        expected = tuple(.125*.9*(n+1)/(.001*32768)*
                         math.sin(2*math.pi*256*(n+1)/32768)
                         for n in range(32))
        self.assert_signal(actual[:32], expected)

    def test_note_reuse(self):
        for opcode in ("fmbell", "fmrhode", "fmwurlie"):
            with self.subTest(opcode=opcode):
                actual = self.render(opcode, depth=.3, score=
                                     "i 1 0 .125 256\n"
                                     "i 1 .25 .125 440\n"
                                     "i 1 .5 .125 256")
                self.assert_signal(actual[:4096], actual[16384:20480])
                fresh = self.render(opcode, depth=.3, score="i 1 0 .125 440")
                self.assert_signal(actual[8192:12288], fresh)

    def test_partial_blocks_and_scale(self):
        for opcode in ("fmbell", "fmrhode", "fmwurlie"):
            with self.subTest(opcode=opcode):
                reference = self.render(opcode, depth=.3)
                actual = self.render(opcode, depth=.3, offset=3)
                self.assertEqual(actual[:3], (0.0,)*3)
                self.assertEqual(max(map(abs, actual[4099:]), default=0), 0)
                self.assert_signal(actual[3:4099], reference)
                self.assert_signal(self.render(opcode, depth=.3, block=1), reference)
                self.assert_signal(self.render(opcode, depth=.3, fullscale=32768),
                                   reference)


if __name__ == "__main__":
    unittest.main()
