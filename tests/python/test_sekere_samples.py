"""Check PhISEM sample state, default object counts and amplitude units."""

import array
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
import wave


class SekereSampleTests(unittest.TestCase):
    def test_shared_percussion_models(self):
        default = Path(__file__).resolve().parents[2] / "build" / "csound"
        executable = Path(os.environ.get("CSOUND_TEST_EXECUTABLE", default)).resolve()
        if not executable.is_file():
            self.skipTest("Set CSOUND_TEST_EXECUTABLE to the built csound command")
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)

            def render(opcode, block, fullscale=1, count=None, offset=0):
                argument = "" if count is None else f",{count}"
                # Reset the seed on each note to check instance reuse as well.
                csd = f'''<CsoundSynthesizer>
<CsOptions>
-d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = {block}
nchnls = 1
0dbfs = {fullscale}
instr 1
  seed 123
  aSignal {opcode} .01*0dbfs,.01{argument}
  out aSignal
endin
</CsInstruments>
<CsScore>
i 1 {offset/8192} .2508544921875
i 1 {0.5+offset/8192} .2508544921875
e
</CsScore>
</CsoundSynthesizer>
'''
                source = root / "test.csd"
                output = root / "output.wav"
                source.write_text(csd)
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
                return first

            for opcode, count in (("sekere", 64), ("sandpaper", 128), ("stix", 30)):
                with self.subTest(opcode=opcode):
                    reference = render(opcode, 1)
                    self.assertTrue(any(reference), f"{opcode}: silent default")
                    for block in (16, 32):
                        self.assertEqual(reference, render(opcode, block),
                                         f"{opcode}: depends on block size {block}")
                    self.assertEqual(reference, render(opcode, 16, offset=3),
                                     f"{opcode}: partial-block state")
                    self.assertEqual(reference, render(opcode, 16, count=count),
                                     f"{opcode}: explicit default differs")
                    self.assertEqual(reference, render(opcode, 16, fullscale=32768),
                                     f"{opcode}: amplitude depends on 0dbfs")


if __name__ == "__main__":
    unittest.main()
