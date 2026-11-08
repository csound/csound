"""Check fmmetal frequency units and note state."""

import math
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest


class FmmetalTests(unittest.TestCase):
    def setUp(self):
        repo = Path(__file__).resolve().parents[2]
        self.executable = Path(os.environ.get("CSOUND_TEST_EXECUTABLE",
                                             repo / "build" / "csound")).resolve()
        if not self.executable.is_file():
            self.skipTest("Set CSOUND_TEST_EXECUTABLE to the built csound command")
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)

    def render(self, depth=0, block=16, offset=0, fullscale=1, sr=32768,
               sizes=(1024,)*4, triangle=False, index=.5, score=None,
               change_pitch=False):
        source = self.root / "test.csd"
        wave = self.root / "output.wav"
        if score is None:
            score = f"i 1 {offset/sr} .125 256"
        tables = []
        for number, size in enumerate(sizes, 1):
            shape = f"-7,0,{size//4},1,{size//2},-1,{size//4},0" if triangle else "10,1"
            tables.append(f"giWave{number} ftgen {number},0,{size},{shape}")
        table_source = "\n".join(tables)
        source.write_text(f'''<CsoundSynthesizer>
<CsOptions>
-d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr={sr}
ksmps={block}
nchnls=1
0dbfs={fullscale}
{table_source}
giVibrato ftgen 5,0,128,10,1
instr 1
kFrequency init p4
kCycle timeinstk
if {int(change_pitch)} == 1 && kCycle > 64 then
  kFrequency = 2*p4
endif
aOut fmmetal .125*0dbfs,kFrequency,{index},.5,{depth},6,giWave1,giWave2,giWave3,giWave4,giVibrato
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

    def test_carrier_frequency(self):
        gain = .125 * .933033**7
        for sr in (16384, 32768):
            for size in (1024, 2048):
                with self.subTest(sr=sr, size=size):
                    actual = self.render(sr=sr, sizes=(size,)*4, index=0)
                    expected = tuple(gain*math.sin(2*math.pi*256*(n+1)/sr)
                                     for n in range(1024, len(actual)))
                    self.assert_signal(actual[1024:], expected)

    def test_control_pitch_change(self):
        actual = self.render(index=0, change_pitch=True)
        gain = .125 * .933033**7
        expected = tuple(gain*math.sin(2*math.pi*512*(n-1023)/32768)
                         for n in range(1024, len(actual)))
        self.assert_signal(actual[1024:], expected)

    def test_operator_table_lengths(self):
        for depth in (0, .3):
            with self.subTest(depth=depth):
                # Linear tables describe the same wave exactly at each size.
                reference = self.render(triangle=True, depth=depth)
                for sizes in ((128,256,512,1024), (1024,512,256,128)):
                    self.assert_signal(self.render(triangle=True, sizes=sizes,
                                                   depth=depth), reference)

    def test_note_reuse(self):
        actual = self.render(depth=.3, score="i 1 0 .125 256\n"
                             "i 1 .25 .125 440\n"
                             "i 1 .5 .125 256")
        self.assert_signal(actual[:4096], actual[16384:20480])
        fresh = self.render(depth=.3, score="i 1 0 .125 440")
        self.assert_signal(actual[8192:12288], fresh)

    def test_partial_blocks_and_scale(self):
        reference = self.render(depth=.3)
        actual = self.render(depth=.3, offset=3)
        self.assertEqual(actual[:3], (0.0,)*3)
        self.assertEqual(max(map(abs, actual[4099:]), default=0), 0)
        self.assert_signal(actual[3:4099], reference)
        self.assert_signal(self.render(depth=.3, block=1), reference)
        self.assert_signal(self.render(depth=.3, fullscale=32768), reference)


if __name__ == "__main__":
    unittest.main()
