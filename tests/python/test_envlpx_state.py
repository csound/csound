"""Check envlpx and envlpxr rise endpoints and reused envelope state."""

import math
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest


class EnvlpxTests(unittest.TestCase):
    def setUp(self):
        repo = Path(__file__).resolve().parents[2]
        self.executable = Path(os.environ.get("CSOUND_TEST_EXECUTABLE",
                                             repo / "build" / "csound")).resolve()
        if not self.executable.is_file():
            self.skipTest("Set CSOUND_TEST_EXECUTABLE to the built csound command")
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)

    def render(self, opcode="envlpx", rate="a", length=10, block=1, offset=0,
               rise=32, duration=256, decay=64, steady=1, modifier=0,
               note_length=384, alias=False, score=None, independent=0):
        source = self.root / "test.csd"
        wave = self.root / "output.wav"
        arguments = f"{rise/8192},"
        if opcode == "envlpx":
            arguments += f"{duration/8192},"
        arguments += f"p4,giRise,{steady},.01,{modifier}"
        if opcode == "envlpxr":
            arguments += f",{independent}"
        if rate == "k":
            envelope = f"kEnv {opcode} .125,{arguments}\naOut upsamp kEnv"
        else:
            output = "aInput" if alias else "aOut"
            envelope = f"aInput upsamp .125\n{output} {opcode} aInput,{arguments}"
        if score is None:
            score = f"i 1 {offset/8192} {note_length/8192} {decay/8192}"
        output = "aInput" if alias and rate == "a" else "aOut"
        source.write_text(f'''<CsoundSynthesizer>
<CsOptions>
-d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr=8192
ksmps={block}
nchnls=1
0dbfs=1
giRise ftgen 0,0,-{length},-7,0,{length},1
instr 1
{'xtratim .0078125' if opcode == 'envlpxr' else ''}
{envelope}
out {output}
endin
</CsInstruments>
<CsScore>
{score}
e
</CsScore>
</CsoundSynthesizer>
''')
        result = subprocess.run([str(self.executable), "-W", "-f", "-o",
                                 str(wave), str(source)], capture_output=True,
                                text=True, cwd=self.root, timeout=30)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
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

    def test_rise_and_decay(self):
        expected = [.125*n/32 if n < 32 else
                    (.125 if n < 192 else .125*.01**((n-192)/64))
                    for n in range(384)]
        for rate in ("a", "k"):
            for length in (10, 16, 30):
                with self.subTest(rate=rate, length=length):
                    actual = self.render(rate=rate, length=length)
                    self.assert_signal(actual[:384], expected)

    def test_curve_and_partial_blocks(self):
        for opcode in ("envlpx", "envlpxr"):
            for steady, modifier in ((.5, .3), (2, -.3)):
                with self.subTest(opcode=opcode, steady=steady):
                    reference = self.render(opcode, length=16, block=16,
                                            steady=steady, modifier=modifier,
                                            offset=3, note_length=389)
                    actual = self.render(opcode, length=10, block=16,
                                         steady=steady, modifier=modifier,
                                         offset=3, note_length=389, alias=True)
                    self.assert_signal(actual, reference)
                    self.assertEqual(actual[:3], (0.0,)*3)
                    self.assertEqual(max(map(abs, actual[392+64:]), default=0), 0)

    def test_no_steady_stage(self):
        for rate in ("a", "k"):
            for rise in (0, 32):
                with self.subTest(rate=rate, rise=rise):
                    plain = self.render(rate=rate, rise=rise, duration=rise+64)
                    curved = self.render(rate=rate, rise=rise, duration=rise+64,
                                         steady=.5, modifier=.3)
                    self.assert_signal(curved, plain)
                    self.assertAlmostEqual(curved[rise], .125, delta=2e-6)

    def test_zero_decay_note_reuse(self):
        for opcode, independent in (("envlpx", 0), ("envlpxr", 0),
                                    ("envlpxr", 1)):
            for rate in ("a", "k"):
                with self.subTest(opcode=opcode, rate=rate,
                                  independent=independent):
                    reference = self.render(opcode, rate, decay=0,
                                            independent=independent)
                    actual = self.render(opcode, rate, independent=independent,
                                         score="i 1 0 .046875 .0078125\n"
                                               "i 1 .125 .046875 0")
                    count = 448 if opcode == "envlpxr" else 384
                    self.assert_signal(actual[1024:1024+count], reference[:count])
                    self.assertAlmostEqual(reference[count-1], .125, delta=2e-6)

    def test_nonzero_independence_flag(self):
        for rate in ("a", "k"):
            with self.subTest(rate=rate):
                reference = self.render("envlpxr", rate, decay=32, independent=1)
                actual = self.render("envlpxr", rate, decay=32, independent=.5)
                self.assert_signal(actual, reference)

    def test_release_at_rise_end(self):
        for length in (10, 16):
            with self.subTest(length=length):
                control = self.render("envlpxr", "k", length, note_length=31)
                audio = self.render("envlpxr", "a", length, note_length=31)
                self.assert_signal(audio, control)


if __name__ == "__main__":
    unittest.main()
