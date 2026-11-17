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
giConstant ftgen 0, 0, 8, -2, 1, 1, 1, 1, 1, 1, 1, 1
gkChecks init 0
instr Detector
  ; p4 is the detector cutoff; p5 is the constant input level.
  ; The input clips at the table endpoint, exposing the RMS estimate.
  aInput init p5
  aOutput distort aInput, 1, giShape, p4
  kOutput downsamp aOutput
  kSample init 0
  if kSample == sr-1 then
    ; At ksmps=1, output uses the preceding sample's level, with an RMS floor.
    ; p6 is the closed-form detector level after 47999 samples.
    ; Keep the reference independent of rms and its build precision.
    kExpected = max(p6, 1/32768)
    if !(abs(kOutput/kExpected-1) < .00001) then
      printks "Cutoff %g, input %g: expected %g, got %g\n", 0, p4, p5, kExpected, kOutput
      exitnowk -1
    endif
    gkChecks += 1
  endif
  kSample += 1
endin
instr LevelChanges
  ; A constant shaper exposes the detector even during silence.
  kSample init 0
  aInput = (kSample < 32 ? 1e20 : (kSample < 160 ? 0 : 1))
  aOutput distort aInput, 1, giConstant, sr/2
  kOutput downsamp aOutput
  if kSample == 80 then
    ; Previous level: 32 loud samples, followed by 48 zero samples.
    if !(abs(kOutput/p4-1) < .00001) then
      printks "Detector decay: expected %g, got %g\n", 0, p4, kOutput
      exitnowk -1
    endif
    gkChecks += 1
  elseif kSample == 159 then
    if !(abs(kOutput/(1/32768)-1) < .00001) then
      exitnowk -1
    endif
    gkChecks += 1
  elseif kSample == 200 then
    if !(abs(kOutput-1) < .00001) then
      printks "Detector recovery: expected 1, got %g\n", 0, kOutput
      exitnowk -1
    endif
    gkChecks += 1
  endif
  kSample += 1
endin
instr CheckCompletion
  if i(gkChecks) != 8 then
    prints "Not all detector checks completed\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Reference: level*sqrt(1-(1-c1)^47999), where
; c1=2*s/(sqrt(1+s*s)+s), s=abs(sin(pi*cutoff/sr)).
; Very low cutoff, ordinary cutoff, Nyquist, and zero cutoff.
i "Detector" 0 1.01 .0001 1 0.025062084873724241
i "Detector" 0 1.01 10 1 1
i "Detector" 0 1.01 24000 1 1
i "Detector" 0 1.01 0 1 0
; Squared input exceeds float range, but the RMS and output remain finite.
i "Detector" 0 1.01 10 1e20 1e+20
i "LevelChanges" 0 .01 42.3415842754171364407182576705407443948169381061396688738282
i "CheckCompletion" 1.02 .01
e
</CsScore>
</CsoundSynthesizer>
