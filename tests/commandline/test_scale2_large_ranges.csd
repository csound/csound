<CsTest>
description = "scale2 maps large finite input and output ranges without overflow"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 1
nchnls = 1
0dbfs = 1
gkChecks init 0

instr CheckRanges
  kStep init 0
  ; Visit both endpoints and the midpoint. The two extra values check clamping.
  kInputValues[] fillarray -3, -2, 0, 2, 3
  kPosition = (kStep-1)/2
  kExpected = limit(kPosition, 0, 1)
  kLargeInput = kInputValues[kStep]*1e38
  kFromLarge scale2 kLargeInput, 0, 1, -2e38, 2e38
  kToLarge scale2 kPosition, -2e38, 2e38
  kReversed scale2 kPosition, 2e38, -2e38
  ; Scale the outputs before comparing, so the assertions cannot overflow.
  kExpectedOutput = 4*kExpected-2
  if !(abs(kFromLarge-kExpected) < .000001) then
    printks "scale2 large input at step %g: expected %g, got %g\n", 0, kStep, kExpected, kFromLarge
    exitnowk -1
  endif
  if !(abs(kToLarge/1e38-kExpectedOutput) < .000001) then
    printks "scale2 large output at step %g: expected %g, got %g (scaled by 1e38)\n", 0, kStep, kExpectedOutput, kToLarge/1e38
    exitnowk -1
  endif
  if !(abs(kReversed/1e38+kExpectedOutput) < .000001) then
    printks "scale2 reversed output at step %g: expected %g, got %g (scaled by 1e38)\n", 0, kStep, -kExpectedOutput, kReversed/1e38
    exitnowk -1
  endif
  kStep += 1
  gkChecks += 1
endin

instr CheckCompletion
  if i(gkChecks) != 5 then
    prints "All five range positions must be checked\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "CheckRanges" 0 [5/1024]
i "CheckCompletion" [8/1024] [1/1024]
e
</CsScore>
</CsoundSynthesizer>
