<CsTest>
description = "Sliding pvsanal retains small bins when a ring sample does not change"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 1
nchnls = 1
0dbfs = 1
gkChecks init 0

instr CheckUnchanged
 iTiny = 2^(-p4)
 kSample init 0
 kPosition = kSample % 4
 kInput = (kPosition == 0 ? -1 : (kPosition == 1 ? 1 : (kPosition == 2 ? iTiny : 0)))
 aInput = kInput
 fAnalysis pvsanal aInput, 4, 1, 4, 9
 aMagnitude, aFrequency pvsbin fAnalysis, 0
 kMagnitude downsamp aMagnitude
 if kSample >= 3 && kMagnitude != iTiny then
  printks "pvsanal unchanged: exponent=%g sample=%g magnitude=%.12g expected=%.12g\n", 0, p4, kSample, kMagnitude, iTiny
  exitnowk(-1)
 endif
 kSample += 1
 if kSample == 20 then
  gkChecks += 1
  turnoff
 endif
endin

instr CheckResults
 if i(gkChecks) != 2 then
  prints "pvsanal unchanged checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
; Small enough to expose loss in float and double accumulators.
i "CheckUnchanged" 0 .01 26
i "CheckUnchanged" 0 .01 55
i "CheckResults" .02 .001
e
</CsScore>
</CsoundSynthesizer>
