"""Check grain timing and source positions across source sample rates."""

import os
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest
import wave


class SyncgrainSampleRateTests(unittest.TestCase):
    def test_table_grain_rates(self):
        default = Path(__file__).resolve().parents[2] / "build" / "csound"
        executable = Path(os.environ.get("CSOUND_TEST_EXECUTABLE", default)).resolve()
        if not executable.is_file():
            self.skipTest("Set CSOUND_TEST_EXECUTABLE to the built csound command")
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            rates = (4096, 8192, 16384)
            for rate in rates:
                with wave.open(str(root / f"source-{rate}.wav"), "wb") as output:
                    output.setnchannels(1)
                    output.setsampwidth(2)
                    output.setframerate(rate)
                    output.writeframes(b"".join(struct.pack("<h", 4096 + (i % 1024)*16)
                                               for i in range(rate)))
            tables = "\n".join(f'giSource{rate} ftgen {i+1}, 0, 0, -1, '
                               f'"source-{rate}.wav", 0, 0, 1'
                               for i, rate in enumerate(rates))
            score = []
            for i, rate in enumerate(rates):
                for j, (pitch, advance) in enumerate(((1, 1), (-1, .5), (0, 0))):
                    start = (i*3+j)*.125 + 3/8192
                    score.append(f'i 1 {start} .06298828125 {i+1} {rate} {pitch} {advance}')
            score.append('i 1 1.1253662109375 .06298828125 1 4096 0 1 1')
            csd = """<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0
""" + tables + """
giEnvelope ftgen 10, 0, 16, -7, 1, 16, 1
instr 1
  iSource = p4
  iScale = p5/sr
  iPitch = p6
  iAdvance = p7
  iLength = ftlen(iSource)
  iGrainSamples = (p8 == 1 ? 1 : 32)
  iLoopEnd = (p8 == 1 ? 0 : .0078125)
  iLoopLength = (p8 == 1 ? 1 : 64*iScale)
  iSamples = round(p3*sr)
  kStart init round(p2*sr) % ksmps
  kSample init 0
  ; Also exercise a one-output-sample grain with the default loop span.
  aGrain syncgrain 1, 64, iPitch, iGrainSamples/sr, iAdvance, iSource, giEnvelope, 16
  aLoop syncloop 1, 64, iPitch, iGrainSamples/sr, iAdvance, 0, iLoopEnd, iSource, giEnvelope, 16
  kIndex = 0
  while kIndex < ksmps do
    kExpected = 0
    kLoopExpected = 0
    if kIndex >= kStart && kSample < iSamples then
      kAge = kSample % 128
      kGrain = int(kSample/128)
      if kAge < iGrainSamples then
        kOrigin = kGrain * iGrainSamples * iScale * iAdvance
        kPosition = kOrigin + kAge * iScale * iPitch
        kPosition -= floor(kPosition/iLength)*iLength
        kExpected tablei kPosition, iSource
        kLoopOrigin = kOrigin - floor(kOrigin/iLoopLength)*iLoopLength
        kPosition = kLoopOrigin + kAge * iScale * iPitch
        kPosition -= floor(kPosition/iLength)*iLength
        kLoopExpected tablei kPosition, iSource
      endif
      kSample += 1
    endif
    kActual vaget kIndex, aGrain
    kLoopActual vaget kIndex, aLoop
    kError = abs(kActual - kExpected) + abs(kLoopActual - kLoopExpected)
    if !(kError < .000002) then
      printks "grain mismatch: source_sr=%g pitch=%g advance=%g sample=%g actual=%g expected=%g loop=%g expected_loop=%g\\n", 0, p5, iPitch, iAdvance, kSample, kActual, kExpected, kLoopActual, kLoopExpected
      exitnowk(-1)
    endif
    kIndex += 1
  od
  kStart = 0
  kChecked init 0
  if kSample == iSamples && kChecked == 0 then
    gkChecks += 1
    kChecked = 1
  endif
endin
instr 99
  if i(gkChecks) != 10 then
    prints "grain checks did not complete\\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
""" + "\n".join(score) + """
i 99 1.25 .01
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
