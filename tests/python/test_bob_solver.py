"""Check bob against its four-stage filter equations and RK4 solver."""

import itertools
import math
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest


class BobTests(unittest.TestCase):
    def setUp(self):
        repo = Path(__file__).resolve().parents[2]
        self.executable = Path(os.environ.get("CSOUND_TEST_EXECUTABLE",
                                             repo / "build" / "csound")).resolve()
        if not self.executable.is_file():
            self.skipTest("Set CSOUND_TEST_EXECUTABLE to the built csound command")
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)

    def render(self, rates="kkk", oversampling=2, alias=False, varying=True,
               score="i 1 .0003662109375 .015869140625 0", saturation=1):
        parameters = "aRamp phasor 32" if varying else "aRamp=0"
        arguments = [f"{'a' if rate == 'a' else 'k'}{name}"
                     for rate, name in zip(rates, ("Freq", "Res", "Sat"))]
        output = "aInput" if alias else "aOutput"
        source = self.root / "test.csd"
        wave = self.root / "output.wav"
        source.write_text(f'''<CsoundSynthesizer>
<CsOptions>
-d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr=8192
ksmps=16
nchnls=4
0dbfs=1
instr 1
{parameters}
aFreq=300+256*aRamp
aRes=.5+aRamp
aSat={saturation}+aRamp
kFreq downsamp aFreq
kRes downsamp aRes
kSat downsamp aSat
aInput upsamp .125
{output} bob aInput,{','.join(arguments)},{oversampling},p4
outq {output},aFreq,aRes,aSat
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
                samples = struct.unpack_from(f"<{size//4}f", raw, position + 8)
                self.assertTrue(all(map(math.isfinite, samples)))
                return [samples[channel::4] for channel in range(4)]
            position += 8 + size + (size & 1)
        self.fail("No WAV samples")

    @staticmethod
    def reference(parameters, oversampling):
        state = [0.0] * 4
        result = []
        dt = 1 / (8192 * oversampling)
        for frequency, resonance, saturation in parameters:
            def derivative(values):
                if saturation == 0:
                    return [0.0] * 4
                stages = [.125 - resonance * values[3]] + list(values)
                clipped = [saturation * math.tanh(x / saturation) for x in stages]
                return [2 * math.pi * frequency * (a-b)
                        for a, b in zip(clipped, clipped[1:])]

            for _ in range(oversampling):
                slopes = []
                for fraction in (0, .5, .5, 1):
                    candidate = [x + fraction * dt * slope for x, slope in
                                 zip(state, slopes[-1])] if slopes else state
                    slopes.append(derivative(candidate))
                state = [x + dt * (a + 2*b + 2*c + d) / 6
                         for x, a, b, c, d in zip(state, *slopes)]
            result.append(state[3])
        return result

    def assert_close(self, actual, expected):
        self.assertEqual(len(actual), len(expected))
        self.assertLess(max(abs(a-b) for a, b in zip(actual, expected)), 2e-6)

    def test_solver_and_parameter_inputs(self):
        # Full blocks allow exact comparison of all eight parameter-rate mixes.
        score = "i 1 0 .03125 0"
        ramps = [n / 256 for n in range(256)]
        audio_parameters = [[300 + 256*r for r in ramps],
                            [.5+r for r in ramps], [1+r for r in ramps]]
        for rates in itertools.product("ka", repeat=3):
            for oversampling in (1, 2, 4):
                with self.subTest(rates=rates, oversampling=oversampling):
                    actual = self.render(rates, oversampling, score=score)
                    parameters = [[column[n if rate == "a" else n//16*16]
                                   for column, rate in zip(audio_parameters, rates)]
                                  for n in range(256)]
                    self.assert_close(actual[0][:256],
                                      self.reference(parameters, oversampling))
                    for observed, expected in zip(actual[1:], audio_parameters):
                        self.assert_close(observed[:256], expected)

    def test_partial_block_and_input_reuse(self):
        for oversampling, effective in ((0, 2), (.5, 1), (2, 2)):
            for alias in (False, True):
                with self.subTest(oversampling=oversampling, alias=alias):
                    actual = self.render("aaa", oversampling, alias, varying=False)[0]
                    expected = ([0.0]*3 + self.reference([(300, .5, 1)]*130,
                                                        effective))
                    expected += [0.0] * (len(actual)-len(expected))
                    self.assert_close(actual, expected)

    def test_note_state(self):
        for rates in ("kkk", "aaa"):
            with self.subTest(rates=rates):
                reference = self.render(rates, varying=False,
                                        score="i 1 0 .0625 0")[0]
                resumed = self.render(rates, varying=False, score=
                                      "i 1 0 .03125 0\ni 1 .0625 .03125 1")[0]
                self.assert_close(resumed[:256] + resumed[512:768], reference[:512])
                reset = self.render(rates, varying=False, score=
                                    "i 1 0 .03125 0\ni 1 .0625 .03125 0")[0]
                self.assertEqual(reset[:256], reset[512:768])

    def test_zero_saturation(self):
        actual = self.render("aaa", varying=False, saturation=0)[0]
        self.assertEqual(max(map(abs, actual)), 0)
        resumed = self.render("aaa", varying=False, saturation="p5", score=
                              "i 1 0 .03125 0 1\ni 1 .0625 .03125 1 0\n"
                              "i 1 .125 .03125 1 1")[0]
        self.assertEqual(resumed[512:768], (resumed[255],) * 256)
        expected = self.reference([(300, .5, 1)] * 512, 2)
        self.assert_close(resumed[:256] + resumed[1024:1280], expected)


if __name__ == "__main__":
    unittest.main()
