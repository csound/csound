"""Check mv filter cutoff state and audio/control-rate agreement."""

import math
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest


class MvFilterTests(unittest.TestCase):
    def setUp(self):
        repo = Path(__file__).resolve().parents[2]
        self.executable = Path(os.environ.get("CSOUND_TEST_EXECUTABLE",
                                             repo / "build" / "csound")).resolve()
        if not self.executable.is_file():
            self.skipTest("Set CSOUND_TEST_EXECUTABLE to the built csound command")
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)

    def render(self, opcode, rates="kk", score="i 1 0 .125 1000 0",
               changes=False, alias=None):
        channels = 4 if opcode == "mvclpf4" else 1
        outputs = [f"aOut{n}" for n in range(channels)]
        if alias is not None:
            outputs[alias] = "aInput"
        frequency = "aCut" if rates[0] == "a" else "kCut"
        resonance = "aRes" if rates[-1] == "a" else "kRes"
        arguments = f"aInput,{frequency}"
        if opcode != "mvchpf":
            arguments += f",{resonance}"
        arguments += ",p5"
        parameters = "kCut=p4\nkRes=.3"
        if changes:
            parameters = """kCount timeinstk
kCut=(kCount<17 ? 1000 : (kCount<33 ? 0 : (kCount<49 ? -1 : 16000)))
kRes=(kCount<33 ? .3 : .7)"""
        source = self.root / "test.csd"
        output = self.root / "output.wav"
        source.write_text(f'''<CsoundSynthesizer>
<CsOptions>
-d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr=8192
ksmps=16
nchnls={channels}
0dbfs=1
instr 1
{parameters}
aCut upsamp kCut
aRes upsamp kRes
aInput upsamp .125
{','.join(outputs)} {opcode} {arguments}
{'outq' if channels == 4 else 'out'} {','.join(outputs)}
endin
</CsInstruments>
<CsScore>
{score}
e
</CsScore>
</CsoundSynthesizer>
''')
        result = subprocess.run([str(self.executable), "-W", "-f", "-o",
                                 str(output), str(source)], capture_output=True,
                                text=True, cwd=self.root, timeout=30)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        raw = output.read_bytes()
        position = 12
        while position + 8 <= len(raw):
            tag, size = struct.unpack_from("<4sI", raw, position)
            if tag == b"data":
                samples = struct.unpack_from(f"<{size//4}f", raw, position + 8)
                self.assertTrue(all(map(math.isfinite, samples)))
                return samples
            position += 8 + size + (size & 1)
        self.fail("No WAV samples")

    def assert_samples_close(self, actual, expected):
        self.assertEqual(len(actual), len(expected))
        self.assertLess(max(abs(a-b) for a, b in zip(actual, expected)), 1e-6)

    def test_zero_cutoff_note_reuse(self):
        score = "i 1 0 .125 0 0\ni 1 .25 .125 1000 0\ni 1 .5 .125 0 0"
        for opcode in ("mvclpf1", "mvclpf2", "mvclpf3", "mvclpf4", "mvchpf"):
            rates = ("kk", "ak") if opcode == "mvchpf" else ("kk", "ak", "ka", "aa")
            for rate in rates:
                with self.subTest(opcode=opcode, rate=rate):
                    samples = self.render(opcode, rate, score)
                    channels = 4 if opcode == "mvclpf4" else 1
                    first = samples[:1024*channels]
                    third = samples[4096*channels:5120*channels]
                    self.assertEqual(first, third)
                    self.assertGreater(max(map(abs, samples[2048*channels:3072*channels])), .01)
                    if opcode != "mvchpf":
                        self.assertEqual(max(map(abs, first[channels-1::channels])), 0)

    def test_parameter_rates_and_input_reuse(self):
        # Start and end inside a block; include positive, zero and saturated cutoff.
        score = "i 1 .0003662109375 .125732421875 1000 0"
        for opcode in ("mvclpf1", "mvclpf2", "mvclpf3", "mvclpf4", "mvchpf"):
            reference = self.render(opcode, "kk", score, changes=True)
            rates = ("ak",) if opcode == "mvchpf" else ("ak", "ka", "aa")
            for rate in rates:
                with self.subTest(opcode=opcode, rate=rate):
                    actual = self.render(opcode, rate, score, changes=True)
                    self.assert_samples_close(actual, reference)
            for alias in range(4 if opcode == "mvclpf4" else 1):
                with self.subTest(opcode=opcode, alias=alias):
                    rate = "ak" if opcode == "mvchpf" else "aa"
                    actual = self.render(opcode, rate, score, changes=True, alias=alias)
                    self.assert_samples_close(actual, reference)

    def test_skip_initialization_preserves_state(self):
        for opcode in ("mvclpf1", "mvclpf2", "mvclpf3", "mvclpf4", "mvchpf"):
            for rate in ("kk", "ak" if opcode == "mvchpf" else "aa"):
                with self.subTest(opcode=opcode, rate=rate):
                    reference = self.render(opcode, rate, "i 1 0 .25 1000 0")
                    resumed = self.render(opcode, rate,
                                          "i 1 0 .125 1000 0\ni 1 .25 .125 1000 1")
                    channels = 4 if opcode == "mvclpf4" else 1
                    joined = resumed[:1024*channels] + resumed[2048*channels:3072*channels]
                    self.assert_samples_close(joined, reference[:2048*channels])


if __name__ == "__main__":
    unittest.main()
