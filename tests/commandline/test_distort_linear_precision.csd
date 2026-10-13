<CsTest>
description = "distort preserves quiet signals with a linear shaper"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1
giShape ftgen 0, 0, 17, -7, -1, 16, 1
gkChecks init 0
instr LinearShape
  ; A linear shaper must pass an unclipped input unchanged, including near zero.
  aInput init p4
  aOutput distort aInput, .001, giShape
  aInPlace = aInput
  aInPlace distort aInPlace, .001, giShape
  kPeriod init 0
  if kPeriod == 10 then
    kSample = 0
    while kSample < ksmps do
      kOutput vaget kSample, aOutput
      kInPlace vaget kSample, aInPlace
      if !(abs(kOutput/p4-1) < .00001 && abs(kInPlace/p4-1) < .00001) then
        printks "Linear shaper: expected %g, got %g (in-place %g)\n", 0, p4, kOutput, kInPlace
        exitnowk -1
      endif
      kSample += 1
    od
    gkChecks += 1
  endif
  kPeriod += 1
endin
instr CheckCompletion
  if i(gkChecks) != 4 then
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "LinearShape" 0 .01 1e-9
i "LinearShape" 0 .01 -1e-9
i "LinearShape" 0 .01 .25
i "LinearShape" 0 .01 -.25
i "CheckCompletion" .02 .01
e
</CsScore>
</CsoundSynthesizer>
