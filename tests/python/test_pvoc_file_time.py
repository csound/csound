"""PVOC file time is opt-in; existing calls retain the output-rate timebase."""

import os
from itertools import product
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
            write_pvx(root / "pvoc.1", range(8))
            for rate in (4096, 8192, 16384):
                for mode, position, numeric in product(
                        (None, 0, 1), (0, 1, 1.5, 3.25, 7, 10), (False, True)):
                    with self.subTest(rate=rate, position=position, mode=mode, numeric=numeric):
                        expected = min(position if mode else position * rate / 8192, 7)
                        suffix = "" if mode is None else f", {mode}"
                        add_suffix = "" if mode is None else f", 1, 0, 0, 0, {mode}"
                        voc_suffix = "" if mode is None else f", 0, 0, 0, 0, {mode}"
                        cross_suffix = "" if mode is None else f", 0, {mode}"
                        write_pvx(root / "reference.pvx", [expected] * 8)
                        write_pvx(root / "pvoc.2", [expected] * 8)
                        source_name = "1" if numeric else '"source.pvx"'
                        reference_name = "2" if numeric else '"reference.pvx"'
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
  kFreq, kAmp pvread kTime, {source_name}, 2{suffix}
  if abs(kFreq - {256 + expected * 64}) > .001 || abs(kAmp - {.0625 + expected * .03125}) > .00001 then
    printks "pvread selected the wrong analysis frame\\n", 0
    exitnowk -1
  endif
  aAdd pvadd kTime, 1, {source_name}, giSine, 1, 2{add_suffix}
  aAddRef pvadd 0, 1, {reference_name}, giSine, 1, 2{add_suffix}
  aVoc pvoc kTime, 1, {source_name}{voc_suffix}
  aVocRef pvoc 0, 1, {reference_name}{voc_suffix}
  pvbufread kTime, {source_name}{suffix}
  aInterp pvinterp kTime, 1, {source_name}, 1, 1, 1, 1, .5, .5{suffix}
  pvbufread 0, {reference_name}{suffix}
  aInterpRef pvinterp 0, 1, {reference_name}, 1, 1, 1, 1, .5, .5{suffix}
  pvbufread kTime, {source_name}{suffix}
  aCross pvcross kTime, 1, {source_name}, .5, .5{cross_suffix}
  pvbufread 0, {reference_name}{suffix}
  aCrossRef pvcross 0, 1, {reference_name}, .5, .5{cross_suffix}
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
                        result = subprocess.run([executable, path.name], cwd=root,
                                                capture_output=True, text=True)
                        self.assertEqual(result.returncode, 0,
                                         result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
