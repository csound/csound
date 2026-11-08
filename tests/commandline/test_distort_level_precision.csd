<CsTest>
description = "distort tracks low-cutoff and large RMS levels"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 1
nchnls = 1
0dbfs = 1
; A linear shaper with an extended guard point.
giShape ftgen 0, 0, 17, -7, -1, 16, 1
gkChecks init 0
instr Detector
  ; p4 is the detector cutoff; p5 is the constant input level.
  ; The input clips at the table endpoint, exposing the RMS estimate.
  aInput init p5
  aOutput distort aInput, 1, giShape, p4
  kOutput downsamp aOutput
  kLevel rms aInput, p4
  kPreviousLevel init 0
  kSample init 0
  if kSample == sr-1 then
    ; At ksmps=1, output uses the preceding sample's level, with an RMS floor.
    kExpected = max(kPreviousLevel, 1/32768)
    if !(abs(kOutput/kExpected-1) < .00001) then
      printks "Cutoff %g, input %g: expected %g, got %g\n", 0, p4, p5, kExpected, kOutput
      exitnowk -1
    endif
    gkChecks += 1
  endif
  kPreviousLevel = kLevel
  kSample += 1
endin
instr CheckCompletion
  if i(gkChecks) != 5 then
    prints "Not all detector checks completed\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Very low cutoff, ordinary cutoff, Nyquist, and zero cutoff.
i "Detector" 0 1.01 .0001 1
i "Detector" 0 1.01 10 1
i "Detector" 0 1.01 24000 1
i "Detector" 0 1.01 0 1
; Squared input exceeds float range, but the RMS and output remain finite.
i "Detector" 0 1.01 10 1e20
i "CheckCompletion" 1.02 .01
e
</CsScore>
</CsoundSynthesizer>
