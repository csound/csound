"""Check moog filter initialization and sweep control."""

import math
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest


class MoogTests(unittest.TestCase):
    def setUp(self):
        repo = Path(__file__).resolve().parents[2]
        self.executable = Path(os.environ.get("CSOUND_TEST_EXECUTABLE",
                                             repo / "build" / "csound")).resolve()
        if not self.executable.is_file():
            self.skipTest("Set CSOUND_TEST_EXECUTABLE to the built csound command")
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)

    def render(self, rate=0, q=.85, block=16, offset=0, fullscale=1,
               pause=None, score=None, q_after=None):
        source = self.root / "test.csd"
        wave = self.root / "output.wav"
        if score is None:
            score = f"i 1 {offset/32768} .125 256 {q}"
        pause_code = ""
        if pause is not None:
            pause_code = f"if kCycle > 64 && kCycle <= 128 then\n kRate = {pause}\nendif"
        q_code = ""
        if q_after is not None:
            q_code = f"if kCycle > 64 then\n kQ = {q_after}\nendif"

        source.write_text(f'''<CsoundSynthesizer>
<CsOptions>
-d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr=32768
ksmps={block}
nchnls=1
0dbfs={fullscale}
giAttack ftgen 1,0,128,-2,0
giSine ftgen 2,0,512,10,1
instr 1
kCycle timeinstk
kRate = {rate}
{pause_code}
kQ = p5
{q_code}
aOut moog .01*0dbfs,p4,kQ,kRate,0,0,giAttack,giSine,giSine
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

    def test_zero_sweep_rate(self):
        held = self.render(rate=0)
        almost_held = self.render(rate=1e-12)
        self.assertGreater(max(map(abs, held)), .0001)
        self.assert_signal(held, almost_held)
        moving = self.render(rate=.0002)
        self.assertGreater(max(abs(a-b) for a,b in zip(held, moving)), .001)

    def test_zero_q(self):
        self.assert_signal(self.render(q=0), self.render(q=1e-12))

    def test_q_change_with_zero_sweep_rate(self):
        changed = self.render(rate=0, q_after=.8)
        self.assert_signal(changed, self.render(rate=1e-12, q_after=.8))
        held = self.render(rate=0)
        self.assert_signal(changed[:1024], held[:1024])
        self.assertGreater(max(abs(a-b) for a, b in
                               zip(changed[1024:], held[1024:])), .001)

    def test_pause_and_resume(self):
        paused = self.render(rate=.0002, pause=0)
        self.assert_signal(paused, self.render(rate=.0002, pause=1e-12))
        moving = self.render(rate=.0002)
        self.assert_signal(paused[:1024], moving[:1024])
        self.assertGreater(max(abs(a-b) for a,b in zip(paused[2048:],
                                                      moving[2048:])), .001)

    def test_note_reuse(self):
        for rate in (0, .0002):
            with self.subTest(rate=rate):
                actual = self.render(rate=rate, score="i 1 0 .125 256 .85\n"
                                     "i 1 .25 .125 440 .8\n"
                                     "i 1 .5 .125 256 .85")
                self.assert_signal(actual[:4096], actual[16384:20480])
                fresh = self.render(rate=rate, score="i 1 0 .125 440 .8")
                self.assert_signal(actual[8192:12288], fresh)

    def test_partial_blocks_and_scale(self):
        for rate in (0, .0002):
            with self.subTest(rate=rate):
                reference = self.render(rate=rate)
                actual = self.render(rate=rate, offset=3)
                self.assertEqual(actual[:3], (0.0,)*3)
                self.assertEqual(max(map(abs, actual[4099:]), default=0), 0)
                self.assert_signal(actual[3:4099], reference)
                self.assert_signal(self.render(rate=rate, block=1), reference)
                self.assert_signal(self.render(rate=rate, fullscale=32768), reference)


if __name__ == "__main__":
    unittest.main()
