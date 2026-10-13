<CsTest>
description = "pvsanal reinitializes from FFT to sliding analysis with hop 10 and correct metadata"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 20
nchnls = 1
0dbfs = 1
gkChecks init 0

instr CheckReinit
 setksmps p4
 kSize init 128
 kHop init 32
 kWindow init 1
 kCycle init 0
 kCycle += 1
 if kCycle == 3 then
  kSize = 64
  kHop = 10
  kWindow = 6
  reinit ANALYZE
 endif
 aInput oscili .25, 137
ANALYZE:
 ; Start with FFT analysis, then reuse its buffers for a smaller sliding
 ; transform. Hop 10 must select sliding analysis at every block size.
 fActual pvsanal aInput, i(kSize), i(kHop), i(kSize), i(kWindow)
 fReference pvsanal aInput, 64, 1, 64, 6
 iHop, iBins, iWindowSize, iFormat pvsinfo fActual
 iExpectedHop = (i(kHop) == 10 ? 1 : 32)
 if iHop != iExpectedHop || iBins != i(kSize)/2+1 || iWindowSize != i(kSize) || iFormat != 0 then
  prints "pvsanal metadata: hop=%g bins=%g window=%g format=%g\n", iHop, iBins, iWindowSize, iFormat
  exitnow(-1)
 endif
 rireturn
 aMagnitude, aFrequency pvsbin fActual, 1
 aReferenceMagnitude, aReferenceFrequency pvsbin fReference, 1
 if kCycle >= 3 then
  ; Compare every sample, including the first frame after reinitialization.
  kIndex = 0
  while kIndex < ksmps do
   kMagnitude vaget kIndex, aMagnitude
   kReferenceMagnitude vaget kIndex, aReferenceMagnitude
   kFrequency vaget kIndex, aFrequency
   kReferenceFrequency vaget kIndex, aReferenceFrequency
   if !(abs(kMagnitude-kReferenceMagnitude) < .0001 && abs(kFrequency-kReferenceFrequency) < .01) then
    printks "pvsanal reinit: ksmps=%g cycle=%g sample=%g magnitude=%g/%g frequency=%g/%g\n", 0, ksmps, kCycle, kIndex, kMagnitude, kReferenceMagnitude, kFrequency, kReferenceFrequency
    exitnowk(-1)
   endif
   kIndex += 1
  od
 endif
 if kCycle == 100 then
  gkChecks += 1
 endif
endin

instr CheckResults
 if i(gkChecks) != 3 then
  prints "pvsanal reinit checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
; Blocks smaller than, equal to, and larger than hop 10.
i "CheckReinit" 0 .3 1
i "CheckReinit" 0 .3 10
i "CheckReinit" 0 .3 20
i "CheckResults" .31 .01
e
</CsScore>
</CsoundSynthesizer>
