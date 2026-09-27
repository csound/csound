<CsTest>
description = "Exponential breakpoints retain absolute timing after fractional intervals"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 32
ksmps = 4
nchnls = 1
0dbfs = 1
gkChecks init 0

instr CheckBreakpoints
  setksmps p4
  ; Four intervals of 1.5 control periods must end at period 6, not 8.
  kCurve expsegb 1, 1.5/kr, 2, 3/kr, 4, 4.5/kr, 8, 6/kr, 16
  aCurve expsegb 1, 1.5/kr, 2, 3/kr, 4, 4.5/kr, 8, 6/kr, 16
  aSampleCurve expsegba 1, 1.5/kr, 2, 3/kr, 4, 4.5/kr, 8, 6/kr, 16
  kAudio downsamp aCurve
  kSampleAudio downsamp aSampleCurve
  ; These two times round to the same update. Jump to 4 and hold it.
  kHold expsegb 1, 1.5/kr, 2, 1.6/kr, 4
  aHold expsegb 1, 1.5/kr, 2, 1.6/kr, 4
  aSampleHold expsegba 1, 1.5/kr, 2, 1.6/kr, 4
  kAudioHold downsamp aHold
  kSampleAudioHold downsamp aSampleHold
  kStep init 0
  if kStep == 3 || kStep == 6 then
    kExpected = (kStep == 3 ? 4 : 16)
    if !(abs(kCurve-kExpected) < .00001 && abs(kAudio-kExpected) < .00001 && abs(kSampleAudio-kExpected) < .00001) then
      printks "ksmps=%g, period=%g: expected %g, got control=%g audio=%g expsegba=%g\n", 0, p4, kStep, kExpected, kCurve, kAudio, kSampleAudio
      exitnowk -1
    endif
    if !(kHold == 4 && kAudioHold == 4 && kSampleAudioHold == 4) then
      printks "Same-update breakpoints must hold 4: got %g/%g/%g\n", 0, kHold, kAudioHold, kSampleAudioHold
      exitnowk -1
    endif
    gkChecks += 1
  endif
  kStep += 1
endin

instr CheckCompletion
  if i(gkChecks) != 4 then
    prints "Both block sizes must reach both breakpoints\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "CheckBreakpoints" 0 1 1
i "CheckBreakpoints" 0 1 4
i "CheckCompletion" 1 .125
e
</CsScore>
</CsoundSynthesizer>
