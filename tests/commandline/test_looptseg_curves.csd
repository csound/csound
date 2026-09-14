<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
 kCurves[] fillarray 0, 1e-8, -1e-8, 1e-20, -1e-20, 2, -2, 1000, -1000
 kMids[] fillarray .5, .5, .5, .5, .5, .2689414213699951, .7310585786300049, 0, 1
 kIndex init 0
 kCurve = kCurves[kIndex]
 ; Change the curve at k-rate while holding the phase fixed.  Check both
 ; segment starts and the middle of rising and falling segments.
 kStart looptseg 0, 0, 0, 0, kCurve, 1, 1, kCurve, 1
 kRise looptseg 0, 0, .25, 0, kCurve, 1, 1, kCurve, 1
 kPeak looptseg 0, 0, .5, 0, kCurve, 1, 1, kCurve, 1
 kFall looptseg 0, 0, .75, 0, kCurve, 1, 1, kCurve, 1
 kExpected = kMids[kIndex]
 if !(abs(kStart) < 1e-6) || !(abs(kPeak - 1) < 1e-6) || \
    !(abs(kRise - kExpected) < 1e-6) || \
    !(abs(kFall - (1 - kExpected)) < 1e-6) then
  printks "FAIL looptseg curve=%g start=%g rise=%g peak=%g fall=%g\n", 0, kCurve, kStart, kRise, kPeak, kFall
  exitnowk(-1)
 endif
 kIndex += 1
 gkChecks += 1
 if kIndex == lenarray(kCurves) then
  turnoff
 endif
endin

instr 99
 if i(gkChecks) != 9 then
  prints "looptseg curve checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 .1
i 99 .1 .01
e
</CsScore>
</CsoundSynthesizer>
