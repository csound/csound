"""Check dripwater frequency controls and note state with a fixed random seed."""

import array
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
import wave


class DripwaterFrequencyTests(unittest.TestCase):
    def test_resonator_frequencies(self):
        default = Path(__file__).resolve().parents[2] / "build" / "csound"
        executable = Path(os.environ.get("CSOUND_TEST_EXECUTABLE", default)).resolve()
        if not executable.is_file():
            self.skipTest("Set CSOUND_TEST_EXECUTABLE to the built csound command")
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)

            def render(frequencies=None, block=16, rate=8192, offset=0, fullscale=1):
                arguments = "" if frequencies is None else "," + ",".join(
                    str(value) for value in frequencies)
                source = root / "test.csd"
                output = root / "output.wav"
                source.write_text(f'''<CsoundSynthesizer>
<CsOptions>
-d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = {rate}
ksmps = {block}
nchnls = 1
0dbfs = {fullscale}
instr 1
  seed 123
  aSignal dripwater .01*0dbfs,.01,128,.9,0{arguments}
  out aSignal
endin
</CsInstruments>
<CsScore>
i 1 {offset/rate} {2055/rate}
i 1 {(4096+offset)/rate} {2055/rate}
e
</CsScore>
</CsoundSynthesizer>
''')
                result = subprocess.run([str(executable), "-W", "-s", "-o",
                                         str(output), str(source)], cwd=root,
                                        capture_output=True, text=True, timeout=30)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                with wave.open(str(output), "rb") as sound:
                    self.assertEqual(sound.getsampwidth(), 2)
                    samples = array.array("h", sound.readframes(sound.getnframes()))
                # Stay before the control-rate dettack countdown expires.
                first = samples[offset:offset+1024]
                second = samples[4096+offset:4096+offset+1024]
                self.assertEqual(first, second, "reused note state")
                self.assertGreater(max(map(abs, first)), 100, "silent output")
                return first

            defaults = (450, 600, 750)
            reference = render()
            self.assertEqual(reference, render(defaults), "explicit defaults")
            self.assertEqual(reference, render((0, 0, 0)), "zero selects defaults")
            for index in range(3):
                frequencies = list(defaults)
                frequencies[index] *= 2
                with self.subTest(frequency=index):
                    changed = render(frequencies)
                    self.assertGreater(max(abs(a-b) for a, b in zip(reference, changed)),
                                       100, "frequency control has no effect")
            # Keeping every frequency/sample-rate ratio fixed preserves the model.
            self.assertEqual(reference, render((900, 1200, 1500), rate=16384))
            for frequencies in (defaults, (900, 1000, 1500)):
                reference = render(frequencies)
                self.assertEqual(reference, render(frequencies, block=1))
                self.assertEqual(reference, render(frequencies, offset=3))
                self.assertEqual(reference, render(frequencies, fullscale=32768))


if __name__ == "__main__":
    unittest.main()
