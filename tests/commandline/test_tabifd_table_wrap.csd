<CsTest>
description = "tabifd creates its FFT plan and wraps table positions before interpolation"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
; Keep the guard point equal to sample zero for periodic interpolation.
giRamp ftgen 0, 0, -8, -2, 0, .1, .2, .3, .4, .5, .6, .7, 0
gkChecks init 0

instr CheckWrap
 ; At pitch zero the entire frame samples one table position. Its DC
 ; magnitude is that interpolated value, so no FFT reference is needed.
 fFrequency, fPhase tabifd p4/sr, p6, p7, 64, 16, 1, giRamp
 kFrequency[] init 66
 kPhase[] init 66
 kFrame pvs2array kFrequency, fFrequency
 kPhaseFrame pvs2array kPhase, fPhase
 kCycle init 0
 kCycle += 1
 if kCycle == 4 then
  iExpected = abs(p5*p6)
  iPhase = (p5*p6 < 0 ? 3.141592653589793 : 0)
  kError = abs(kFrequency[0]-iExpected) + abs(kPhase[0]-iExpected) + abs(kPhase[1]-iPhase)
  if !(kError < 1e-5) then
   printks "tabifd table wrap: position=%g gain=%g expected=%g actual=%g phase=%g\n", 0, p4, p6, iExpected, kFrequency[0], kPhase[1]
   exitnowk(-1)
  endif
  gkChecks += 1
 endif
endin

instr CheckPitch
 ; Pitch offsets by a whole table length describe the same samples.
 ; Compare all magnitude, frequency and phase fields, including endpoints.
 fReference, fReferencePhase tabifd 7.5/sr, 1, p4, 64, 16, 0, giRamp
 fWrapped, fWrappedPhase tabifd -.5/sr, 1, p5, 64, 16, 0, giRamp
 kReference[] init 66
 kReferencePhase[] init 66
 kWrapped[] init 66
 kWrappedPhase[] init 66
 kFrame pvs2array kReference, fReference
 kReferenceCount pvs2array kReferencePhase, fReferencePhase
 kWrappedCount pvs2array kWrapped, fWrapped
 kPhaseCount pvs2array kWrappedPhase, fWrappedPhase
 kCycle init 0
 kCycle += 1
 if kCycle == 4 then
  kIndex = 0
  while kIndex < 66 do
   kError = abs(kReference[kIndex]-kWrapped[kIndex]) + abs(kReferencePhase[kIndex]-kWrappedPhase[kIndex])
   if !(kError < .001) then
    printks "tabifd pitch wrap: reference pitch=%g wrapped pitch=%g field=%g error=%g\n", 0, p4, p5, kIndex, kError
    exitnowk(-1)
   endif
   kIndex += 1
  od
  gkChecks += 1
 endif
endin

instr CheckResults
 if i(gkChecks) != 11 then
  prints "tabifd table checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
; Position in samples, expected interpolated value, gain.
i "CheckWrap" 0 .02  0     0    1
i "CheckWrap" 0 .02  1.5  .15   1
i "CheckWrap" 0 .02  7.5  .35   1
i "CheckWrap" 0 .02  -.5  .35   1
i "CheckWrap" 0 .02 -8.5  .35   1
i "CheckWrap" 0 .02 15.5  .35  -2
i "CheckWrap" 0 .02  1.5  .15   0
; A tiny reverse step at zero can round to the table length when wrapped.
; Its output should still be indistinguishable from holding sample zero.
i "CheckWrap" 0 .02  0     0    1 -1e-20
; Reference and equivalent wrapped pitch, in table samples per sample.
i "CheckPitch" 0 .02  .5   8.5
i "CheckPitch" 0 .02 -.5  -8.5
i "CheckPitch" 0 .02  1    8000001
i "CheckResults" .03 .01
e
</CsScore>
</CsoundSynthesizer>
