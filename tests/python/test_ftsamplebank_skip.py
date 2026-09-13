"""Compare sample-bank skip positions with direct GEN01 loads."""

import os
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest
import wave


class FtsamplebankSkipTests(unittest.TestCase):
    def test_init_and_triggered_loads(self):
        default = Path(__file__).resolve().parents[2] / "build" / "csound"
        executable = Path(os.environ.get("CSOUND_TEST_EXECUTABLE", default)).resolve()
        if not executable.is_file():
            self.skipTest("Set CSOUND_TEST_EXECUTABLE to the built csound command")
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            samples = root / "samples"
            samples.mkdir()
            # Adjacent frames differ, exposing even a one-sample skip error.
            for name, period in (("a.wav", 4096), ("b.wav", 2048)):
                with wave.open(str(samples / name), "wb") as output:
                    output.setnchannels(2)
                    output.setsampwidth(2)
                    output.setframerate(8192)
                    frames = [(i % period, -(i % period)) for i in range(32768)]
                    output.writeframes(b"".join(struct.pack("<hh", *f) for f in frames))
            csd = """<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
giChecks init 0
instr 1
  iCount ftsamplebank "samples", p4, p5, 0, 2
  if iCount != 2 then
    prints "ftsamplebank did not load both files\\n"
    exitnow(-1)
  endif
endin
instr 2
  kBase init 200
  kTrigger init 0
  kSkip init .25
  kReloaded init 0
  kTime timeinsts
  if kTime > .03125 && kReloaded == 0 then
    kSkip = .25006103515625
    kTrigger = 1
    kReloaded = 1
  endif
  kCount ftsamplebank "samples", kBase, kTrigger, kSkip, 0, 2
  if kCount != 2 then
    printks "ftsamplebank control load did not find both files\\n", 0
    exitnowk(-1)
  endif
endin
instr 3
  iA ftgen 1000, 0, 0, 1, "samples/a.wav", p5, 0, 2
  iB ftgen 1001, 0, 0, 1, "samples/b.wav", p5, 0, 2
  iFile = 0
  while iFile < 2 do
    iSample = 0
    while iSample < 128 do
      iActual table iSample, p4 + iFile
      iExpected table iSample, 1000 + iFile
      if abs(iActual - iExpected) > .000001 then
        prints "ftsamplebank skip mismatch: table=%g skip=%g sample=%g got=%g expected=%g\\n", p4 + iFile, p5, iSample, iActual, iExpected
        exitnow(-1)
      endif
      iSample += 1
    od
    iFile += 1
  od
  giChecks += 1
endin
instr 99
  if giChecks != 6 then
    prints "ftsamplebank checks did not complete\\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .0078125 100 0
i 1 0 .0078125 110 .25
; Exactly halfway between samples: six-digit score text rounds this down.
i 1 0 .0078125 120 .25006103515625
i 1 0 .0078125 130 1.5
i 2 0 .125
i 3 .015625 .0078125 100 0
i 3 .015625 .0078125 110 .25
i 3 .015625 .0078125 120 .25006103515625
i 3 .015625 .0078125 130 1.5
i 3 .015625 .0078125 200 .25
i 3 .0625 .0078125 200 .25006103515625
i 99 .15625 .0078125
e
</CsScore>
</CsoundSynthesizer>
"""
            (root / "test.csd").write_text(csd)
            result = subprocess.run([str(executable), "test.csd"], cwd=root,
                                    capture_output=True, text=True, timeout=30)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
