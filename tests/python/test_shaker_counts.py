"""Check shaker count normalization and note state."""

import math
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest


class ShakerTests(unittest.TestCase):
    def setUp(self):
        repo = Path(__file__).resolve().parents[2]
        self.executable = Path(os.environ.get("CSOUND_TEST_EXECUTABLE",
                                             repo / "build" / "csound")).resolve()
        if not self.executable.is_file():
            self.skipTest("Set CSOUND_TEST_EXECUTABLE to the built csound command")
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)

    def render(self, beans="8", times=64, block=16, offset=0, fullscale=1,
               decay=0):
        source = self.root / "test.csd"
        output = self.root / "output.wav"
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
kCycle timeinstk
kBeans={beans}
aSignal shaker .05*0dbfs,1000,kBeans,.99,{times},{decay}
out aSignal
endin
</CsInstruments>
<CsScore>
i 1 {offset/8192} 1.0008544921875
i 1 {2+offset/8192} 1.0008544921875
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
                first = samples[offset:offset+8199]
                second = samples[16384+offset:16384+offset+8199]
                self.assertEqual(first, second, "Reused note state differs")
                self.assertEqual(max(map(abs, samples[:offset]), default=0), 0)
                self.assertEqual(max(map(abs, samples[offset+8199:16384+offset])), 0)
                self.assertGreater(max(map(abs, first)), .0001)
                return first
            position += 8 + size + (size & 1)
        self.fail("No WAV samples")

    def test_initial_bean_count(self):
        reference = self.render("1")
        for beans in ("0", ".5", "-2", "1.9"):
            with self.subTest(beans=beans):
                self.assertEqual(self.render(beans), reference)

    def test_changing_bean_count(self):
        # Normalize both newly supplied values and the value used by the cache.
        reference = self.render("(kCycle<17 ? 8 : (kCycle<33 ? 1 : "
                                "(kCycle<49 ? 3 : 1)))")
        actual = self.render("(kCycle<17 ? 8 : (kCycle<33 ? .5 : "
                             "(kCycle<49 ? 3.75 : (kCycle<65 ? 0 : -2))))")
        self.assertEqual(actual, reference)

    def test_repeat_count(self):
        reference = self.render(times=64)
        for times in (3000, 1e20):
            with self.subTest(times=times):
                self.assertEqual(self.render(times=times), reference)
        self.assertEqual(self.render(times=0), self.render(times=-1))
        finite = self.render(times=3)
        self.assertEqual(self.render(times=3.75), finite)
        self.assertNotEqual(finite, reference)

    def test_sample_state_and_amplitude_units(self):
        reference = self.render()
        self.assertEqual(self.render(offset=3)[:4096], reference[:4096])
        # The decay timer runs at control rate; compare before it expires.
        self.assertEqual(self.render(block=1)[:4096], reference[:4096])
        self.assertEqual(self.render(fullscale=32768), reference)
        self.assertEqual(self.render(decay=2)[:4096], reference[:4096])


if __name__ == "__main__":
    unittest.main()
