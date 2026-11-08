"""Check cabasa amplitude units and the shared cabasa/crunch sample state."""

import array
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
import wave


class CabasaAmplitudeTests(unittest.TestCase):
    def test_amplitude_and_sample_state(self):
        default = Path(__file__).resolve().parents[2] / "build" / "csound"
        executable = Path(os.environ.get("CSOUND_TEST_EXECUTABLE", default)).resolve()
        if not executable.is_file():
            self.skipTest("Set CSOUND_TEST_EXECUTABLE to the built csound command")
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)

            def render(opcode, fullscale=1, block=16, offset=0, count=None):
                arguments = "" if count is None else f",{count}"
                source = root / "test.csd"
                output = root / "output.wav"
                source.write_text(f'''<CsoundSynthesizer>
<CsOptions>
-d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr=8192
ksmps={block}
nchnls=1
0dbfs={fullscale}
instr 1
 seed 123
 aSignal {opcode} .01*0dbfs,.01{arguments}
 out aSignal
endin
</CsInstruments>
<CsScore>
i 1 {offset/8192} .2508544921875
i 1 {0.5+offset/8192} .2508544921875
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
                # Compare before the control-rate dettack countdown expires.
                first = samples[offset:offset+1024]
                second = samples[4096+offset:4096+offset+1024]
                self.assertEqual(first, second, f"{opcode}: reused note state")
                self.assertGreater(max(map(abs, first)), 10, f"{opcode}: silent output")
                return first

            for opcode, count in (("cabasa", 512), ("crunch", 7)):
                with self.subTest(opcode=opcode):
                    reference = render(opcode)
                    for fullscale in (32768, .5):
                        self.assertEqual(reference, render(opcode, fullscale=fullscale),
                                         f"{opcode}: amplitude depends on 0dbfs")
                    self.assertEqual(reference, render(opcode, block=1),
                                     f"{opcode}: depends on block size")
                    self.assertEqual(reference, render(opcode, offset=3),
                                     f"{opcode}: partial-block state")
                    self.assertEqual(reference, render(opcode, count=count),
                                     f"{opcode}: explicit default differs")


if __name__ == "__main__":
    unittest.main()
