"""Compare hrtfer with the measured impulse responses in HRTFcompact."""

import math
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import unittest


class HrtferTests(unittest.TestCase):
    def setUp(self):
        repo = Path(__file__).resolve().parents[2]
        self.executable = Path(os.environ.get("CSOUND_TEST_EXECUTABLE",
                                             repo / "build" / "csound")).resolve()
        if not self.executable.is_file():
            self.skipTest("Set CSOUND_TEST_EXECUTABLE to the built csound command")
        self.fixture = repo / "tests" / "soak" / "HRTFcompact"
        self.data = self.fixture.read_bytes()
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        shutil.copyfile(self.fixture, self.root / "HRTFcompact")

    def run_csd(self, block, body, score, error=None):
        source = self.root / "test.csd"
        output = self.root / "output.wav"
        source.write_text(f'''<CsoundSynthesizer>
<CsOptions>
-d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr=44100
ksmps={block}
nchnls=4
0dbfs=1
instr 1
{body}
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
                                text=True, cwd=self.root, timeout=30,
                                env=dict(os.environ, SADIR=str(self.root)))
        messages = result.stdout + result.stderr
        if error is not None:
            self.assertNotEqual(result.returncode, 0, messages)
            self.assertIn(error, messages)
            return None
        self.assertEqual(result.returncode, 0, messages)
        raw = output.read_bytes()
        position = 12
        while position + 8 <= len(raw):
            tag, size = struct.unpack_from("<4sI", raw, position)
            if tag == b"data":
                return struct.unpack_from(f"<{size//4}f", raw, position + 8)
            position += 8 + size + (size & 1)
        self.fail("No WAV samples")

    def check_response(self, block, offset, azimuth, elevation, record, swap=False):
        duration = 2043
        spacing = math.ceil(8192 / block) * block
        starts = [offset + n * spacing for n in range(3)]
        # Keep score times away from integer-sample boundaries in float builds.
        score = "\n".join(f"i 1 {(start+.25)/44100:.17g} {(duration+.25)/44100:.17g}"
                          for start in starts)
        body = f'''
aInput upsamp .125
aLeft,aRight hrtfer aInput,{azimuth},{elevation},"HRTFcompact"
aAliasL init 0
aAliasR init 0
kN=0
while kN<ksmps do
 vaset .125,kN,aAliasL
 vaset .125,kN,aAliasR
 kN+=1
od
aAliasL,aOtherR hrtfer aAliasL,{azimuth},{elevation},"HRTFcompact"
aOtherL,aAliasR hrtfer aAliasR,{azimuth},{elevation},"HRTFcompact"
kCycle init 0
kStart=(kCycle==0 ? {offset} : 0)
kEnd=min(ksmps,{offset+duration}-kCycle*ksmps)
kN=0
while kN<ksmps do
 kL vaget kN,aLeft
 kR vaget kN,aRight
 kAL vaget kN,aAliasL
 kAR vaget kN,aAliasR
 kOL vaget kN,aOtherL
 kOR vaget kN,aOtherR
 if abs(kAL-kL)+abs(kAR-kR)+abs(kOL-kL)+abs(kOR-kR)>0.00000002 then
  printks "input reuse changed output\\n",0
  exitnowk 1
 endif
 if (kN<kStart || kN>=kEnd) && (kL!=0 || kR!=0 || kAL!=0 || kAR!=0) then
  printks "inactive output is not zero\\n",0
  exitnowk 1
 endif
 kN+=1
od
kCycle+=1
outq aLeft,aRight,aAliasL,aAliasR
'''
        samples = self.run_csd(block, body, score)
        notes = [samples[start*4:(start+duration)*4] for start in starts]
        self.assertEqual(notes[0], notes[1], "second note or shared data changed")
        self.assertEqual(notes[0], notes[2], "third note or shared data changed")
        self.assertGreater(max(map(abs, notes[0])), 1e-7)
        # hrtfer emits the first FFT result in the block containing input sample 127.
        latency = max(0, ((127 + offset) // block) * block - offset)
        kernel = struct.unpack_from(">256h", self.data, record * 512)
        for channel in range(2):
            coefficients = kernel[(1-channel if swap else channel)::2]
            total = 0
            response = []
            for value in coefficients:
                total += value
                response.append(.125 * total / (256 * 32768))
            actual = notes[0][channel::4]
            if 128 % block == 0:
                # Preserve legacy latency and truncation of incomplete FFT frames.
                produced = min(duration - latency, (duration // 128) * 128)
                expected = [0.0] * latency + response[:produced]
                expected += [response[-1]] * max(0, produced - 128)
                expected += [0.0] * (duration - len(expected))
                self.assertLess(max(abs(a-b) for a, b in zip(actual, expected)), 2e-8,
                                "output differs from direct convolution")
            else:
                # Larger, unaligned blocks keep the legacy scheduler's gaps.
                self.assertLess(max(abs(a-b) for a, b in zip(actual[:128], response)),
                                2e-8, "first convolution frame differs")
                self.assertLess(max(min(abs(a), abs(a-response[-1]))
                                    for a in actual[128:]), 2e-8,
                                "settled convolution or silent gap differs")

    def test_measured_directions(self):
        # Record positions from the dataset's elevation rows and azimuth grid.
        for azimuth, elevation, record, swap in (
                (0, -40, 0, False), (0, -30, 29, False),
                (90, 0, 152, False), (-90, 0, 152, True),
                (450, 0, 152, False), (180, 50, 327, False),
                (0, 90, 367, False)):
            with self.subTest(azimuth=azimuth, elevation=elevation):
                self.check_response(16, 0, azimuth, elevation, record, swap)

    def test_block_sizes_and_offsets(self):
        for block, offset in ((2, 0), (8, 0), (16, 3), (32, 17),
                              (128, 0), (256, 3), (300, 17)):
            with self.subTest(block=block, offset=offset):
                self.check_response(block, offset, 90, 0, 152)

    def test_missing_data(self):
        (self.root / "HRTFcompact").unlink()
        self.run_csd(16, 'aInput init 0\naL,aR hrtfer aInput,0,0,"HRTFcompact"',
                     "i 1 0 .01", error="hrtfer: cannot load HRTFcompact")


if __name__ == "__main__":
    unittest.main()
