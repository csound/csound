"""PVOC file positions use the analysis rate, not the output sample rate."""

import os
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest


def write_pvx(path, positions):
    """Write valid mono, 128-point amp/frequency frames at 8192 Hz."""
    fmt = struct.pack("<HHIIHHHHI", 0xFFFE, 1, 8192, 16384, 2, 16, 62, 16, 0)
    fmt += bytes.fromhex("c2b912836e2ed411a824de5b96c3ab21")
    fmt += struct.pack("<IIHHHHIIIIff", 1, 32, 0, 0, 1, 1,
                       65, 128, 32, 520, 256, 0)
    frames = []
    for position in positions:
        frame = [value for bin_number in range(65)
                 for value in (0, bin_number * 64)]
        frame[4] = .0625 + position * .03125
        frame[5] = 256 + position * 64
        frames.append(struct.pack("<130f", *frame))
    data = b"".join(frames)
    chunks = b"fmt " + struct.pack("<I", len(fmt)) + fmt
    chunks += b"data" + struct.pack("<I", len(data)) + data
    path.write_bytes(b"RIFF" + struct.pack("<I", len(chunks) + 4) + b"WAVE" + chunks)


class PvocFileTimeTests(unittest.TestCase):
    def test_file_positions_at_different_output_rates(self):
        default = Path(__file__).resolve().parents[2] / "build" / "csound"
        executable = os.environ.get("CSOUND_TEST_EXECUTABLE", str(default))
        if not Path(executable).is_file():
            self.skipTest("Set CSOUND_TEST_EXECUTABLE to the built csound command")
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            write_pvx(root / "source.pvx", range(8))
            for rate in (4096, 8192, 16384):
                for position in (0, 1, 1.5, 3.25, 7, 10):
                    with self.subTest(rate=rate, position=position):
                        expected = min(position, 7)
                        write_pvx(root / "reference.pvx", [expected] * 8)
                        csd = f"""<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = {rate}
ksmps = 16
nchnls = 1
0dbfs = 1
giSine ftgen 0, 0, 1024, 10, 1
instr 1
  kTime = {position / 256}
  kFreq, kAmp pvread kTime, "source.pvx", 2
  if abs(kFreq - {256 + expected * 64}) > .001 || abs(kAmp - {.0625 + expected * .03125}) > .00001 then
    printks "pvread selected the wrong analysis frame\\n", 0
    exitnowk -1
  endif
  aAdd pvadd kTime, 1, "source.pvx", giSine, 1, 2
  aAddRef pvadd 0, 1, "reference.pvx", giSine, 1, 2
  aVoc pvoc kTime, 1, "source.pvx"
  aVocRef pvoc 0, 1, "reference.pvx"
  pvbufread kTime, "source.pvx"
  aInterp pvinterp kTime, 1, "source.pvx", 1, 1, 1, 1, .5, .5
  pvbufread 0, "reference.pvx"
  aInterpRef pvinterp 0, 1, "reference.pvx", 1, 1, 1, 1, .5, .5
  pvbufread kTime, "source.pvx"
  aCross pvcross kTime, 1, "source.pvx", .5, .5
  pvbufread 0, "reference.pvx"
  aCrossRef pvcross 0, 1, "reference.pvx", .5, .5
  aError = abs(aAdd-aAddRef) + abs(aVoc-aVocRef) + abs(aInterp-aInterpRef) + abs(aCross-aCrossRef)
  kError max_k aError, 1, 1
  if kError > .0001 then
    printks "PVOC synthesis selected the wrong analysis frame: %g\\n", 0, kError
    exitnowk -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .03125
e
</CsScore>
</CsoundSynthesizer>
"""
                        path = root / "read.csd"
                        path.write_text(csd)
                        result = subprocess.run([executable, str(path)], cwd=root,
                                                capture_output=True, text=True)
                        self.assertEqual(result.returncode, 0,
                                         result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
