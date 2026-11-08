"""Check waveset cycle boundaries, buffer wrapping, and note state."""

import math
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest


class WavesetTests(unittest.TestCase):
    def setUp(self):
        repo = Path(__file__).resolve().parents[2]
        self.executable = Path(os.environ.get("CSOUND_TEST_EXECUTABLE",
                                             repo / "build" / "csound")).resolve()
        if not self.executable.is_file():
            self.skipTest("Set CSOUND_TEST_EXECUTABLE to the built csound command")
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)

    def render(self, table, repetitions=2, block=16, offset=0, amplitude=.125,
               capacity=1024, alias=False):
        source = self.root / "test.csd"
        output = self.root / "output.wav"
        result = "aInput" if alias else "aResult"
        source.write_text(f'''<CsoundSynthesizer>
<CsOptions>
-d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr=8192
ksmps={block}
nchnls=1
0dbfs=1
giWave ftgen 0,0,-{len(table)},-2,{','.join(map(str,table))}
instr 1
aPhase phasor sr/{len(table)}
aWave table aPhase,giWave,1
aInput = aWave*{amplitude}
{result} waveset aInput,{repetitions},{capacity}
out {result}
endin
</CsInstruments>
<CsScore>
i 1 {offset/8192} .0321044921875
i 1 {.125+offset/8192} .0321044921875
e
</CsScore>
</CsoundSynthesizer>
''')
        run = subprocess.run([str(self.executable), "-W", "-f", "-o",
                              str(output), str(source)], capture_output=True,
                             text=True, cwd=self.root, timeout=30)
        self.assertEqual(run.returncode, 0, run.stdout + run.stderr)
        raw = output.read_bytes()
        position = 12
        while position + 8 <= len(raw):
            tag, size = struct.unpack_from("<4sI", raw, position)
            if tag == b"data":
                samples = struct.unpack_from(f"<{size//4}f", raw, position + 8)
                self.assertTrue(all(map(math.isfinite, samples)))
                first = samples[offset:offset+263]
                self.assertEqual(first, samples[1024+offset:1024+offset+263],
                                 "Reused note state differs")
                self.assertEqual(max(map(abs, samples[:offset]), default=0), 0)
                self.assertEqual(max(map(abs, samples[offset+263:1024+offset])), 0)
                return first
            position += 8 + size + (size & 1)
        self.fail("No WAV samples")

    @staticmethod
    def expected(table, repetitions, amplitude=.125):
        # Split at every second sign change, then concatenate whole cycles.
        source = table * (1024 // len(table))
        boundaries = [0]
        signs = [(n, 1 if sample > 0 else -1)
                 for n, sample in enumerate(source) if sample]
        crossings = [n for (_, previous), (n, sign)
                     in zip(signs, signs[1:]) if previous != sign]
        boundaries.extend(crossings[1::2])
        boundaries.append(len(source))
        output = []
        for begin, end in zip(boundaries, boundaries[1:]):
            output.extend(source[begin:end] * max(1, int(repetitions)))
        return tuple(sample * amplitude for sample in output[:263])

    def assert_signal(self, actual, expected, amplitude=.125):
        self.assertEqual(len(actual), len(expected))
        self.assertLess(max(abs(a-b) for a, b in zip(actual, expected)),
                        abs(amplitude) * 1e-6)

    def test_complete_cycles(self):
        tables = ([1, 2, 3, 4, -1, -2, -3, -4],
                  [-1, -2, -3, -4, 1, 2, 3, 4],
                  [0, 1, 2, 0, -1, -2, 0, 0, 0, 3, 4, 0, -3, -4, 0, 0],
                  [0, -1, -2, 1, 2, -3, 3, 4, 5, -4, -5, -6, 6, -7, 0, 7])
        for table in tables:
            for repetitions in (1, 2, 3, 2.5):
                with self.subTest(table=table, repetitions=repetitions):
                    self.assert_signal(self.render(table, repetitions),
                                       self.expected(table, repetitions))

    def test_partial_blocks_and_aliasing(self):
        table = [1, 2, 3, 4, -1, -2, -3, -4]
        expected = self.expected(table, 3)
        for block, offset, alias in ((1, 0, False), (16, 3, False),
                                     (16, 3, True), (32, 17, False)):
            with self.subTest(block=block, offset=offset, alias=alias):
                self.assert_signal(self.render(table, 3, block, offset,
                                               alias=alias), expected)

    def test_quiet_cycles(self):
        table = [1, 2, 3, 4, -1, -2, -3, -4]
        amplitude = 2**-80
        self.assert_signal(self.render(table, 3, amplitude=amplitude),
                           self.expected(table, 3, amplitude), amplitude)

    def test_ring_wrap_and_full_buffer(self):
        table = [1, 2, 3, 4, -1, -2, -3, -4]
        # A 16-sample buffer fills with the first two cycles.  Repeating
        # them must not replace either cycle or duplicate a boundary sample.
        distinct_cycles = [value * scale for scale in (1, 2, 3, 4)
                           for value in table]
        for block in (1, 8, 16):
            with self.subTest(block=block):
                self.assert_signal(self.render(distinct_cycles, 64,
                                               block=block, capacity=15),
                                   self.expected(distinct_cycles, 64))
        self.assert_signal(self.render(table, 1, capacity=31),
                           self.expected(table, 1))
        self.assert_signal(self.render(table, 1, capacity=0),
                           self.expected(table, 1))


if __name__ == "__main__":
    unittest.main()
