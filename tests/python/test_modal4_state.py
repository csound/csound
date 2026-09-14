"""Check note reuse, tremolo units, and damping in the modal instruments."""

import math
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest


class ModalTests(unittest.TestCase):
    def setUp(self):
        repo = Path(__file__).resolve().parents[2]
        self.executable = Path(os.environ.get("CSOUND_TEST_EXECUTABLE",
                                             repo / "build" / "csound")).resolve()
        if not self.executable.is_file():
            self.skipTest("Set CSOUND_TEST_EXECUTABLE to the built csound command")
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)

    def render(self, opcode, frequency=220, depth=0, speed=3, table_size=128,
               block=16, offset=0, decay=0, fullscale=1, score=None):
        source = self.root / "test.csd"
        wave = self.root / "output.wav"
        arguments = f".125*0dbfs,p4,.5,.5,giStrike,{speed},{depth},giTremolo"
        if opcode != "gogobel":
            arguments += f",{decay/8192}"
        if opcode == "marimba":
            arguments += ",1,100"  # Three strikes, without a random branch.
        if score is None:
            score = f"i 1 {offset/8192} .0321044921875 {frequency}"
        source.write_text(f'''<CsoundSynthesizer>
<CsOptions>
-d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr=8192
ksmps={block}
nchnls=1
0dbfs={fullscale}
giStrike ftgen 0,0,128,-7,0,1,1,127,0
giTremolo ftgen 0,0,{table_size},-7,-1,{table_size//2},1,{table_size//2},-1
instr 1
seed 123
aOut {opcode} {arguments}
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

    def test_note_history(self):
        for opcode in ("vibes", "marimba", "gogobel"):
            for depth in (0, .3):
                for frequency, other in ((220, 1000), (1000, 220)):
                    with self.subTest(opcode=opcode, depth=depth, frequency=frequency):
                        actual = self.render(opcode, depth=depth, score=
                                             f"i 1 0 .0321044921875 {frequency}\n"
                                             f"i 1 .125 .0321044921875 {other}\n"
                                             f"i 1 .25 .0321044921875 {frequency}")
                        self.assertGreater(max(map(abs, actual[:263])), .001)
                        self.assert_signal(actual[:263], actual[2048:2311])

    def test_tremolo_frequency(self):
        for opcode in ("vibes", "marimba", "gogobel"):
            reference = self.render(opcode)[:263]
            for speed in (3, -3, 24579, -24579):
                expected = []
                for n, sample in enumerate(reference):
                    phase = ((n+1)*speed/8192) % 1
                    triangle = -1+4*phase if phase < .5 else 3-4*phase
                    expected.append(sample * (1+.3*triangle))
                for size in (8, 128, 512):
                    with self.subTest(opcode=opcode, speed=speed, size=size):
                        actual = self.render(opcode, depth=.3, speed=speed,
                                             table_size=size)
                        self.assert_signal(actual[:263], expected)

    def test_partial_blocks_and_amplitude(self):
        for opcode in ("vibes", "marimba", "gogobel"):
            with self.subTest(opcode=opcode):
                reference = self.render(opcode, depth=.3)
                actual = self.render(opcode, depth=.3, offset=3)
                self.assertEqual(actual[:3], (0.0,)*3)
                self.assertEqual(max(map(abs, actual[266:]), default=0), 0)
                # Damping runs at control rate; compare before it starts.
                self.assert_signal(actual[3:131], reference[:128])
                self.assert_signal(self.render(opcode, block=1, depth=.3)[:128],
                                   reference[:128])
                self.assert_signal(self.render(opcode, depth=.3, fullscale=32768),
                                   reference)

    def test_vibes_damping(self):
        reference = self.render("vibes", decay=0)
        damped = self.render("vibes", decay=128)
        self.assert_signal(damped[:112], reference[:112])
        self.assertLess(sum(x*x for x in damped[192:240]),
                        .6*sum(x*x for x in reference[192:240]))


if __name__ == "__main__":
    unittest.main()
