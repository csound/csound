<CsTest>
description = "tonela and atonela match tone and atone across ksmps values and cutoffs"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 105
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
  setksmps p4
  kCutoff = p5
  aInput oscili .5, 321
  aLow tone aInput, kCutoff
  aHigh atone aInput, kCutoff
  aLookLow tonela aInput, kCutoff
  aLookHigh atonela aInput, kCutoff
  aError = abs(aLow-aLookLow) + abs(aHigh-aLookHigh)
  kError max_k aError, 1, 1
  if !(kError <= .000001) then
    printks "tonela mismatch: ksmps=%d cutoff=%g error=%g\n", 0, p4, kCutoff, kError
    exitnowk(-1)
  endif
  kFirst init 1
  if kFirst == 1 then
    gkChecks += 1
    kFirst = 0
  endif
endin

instr 99
  if i(gkChecks) != 24 then
    prints "tonela equivalence completed %g checks; expected 24\n", i(gkChecks)
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .006 1 700
i 1 .01 .006 3 700
i 1 .02 .006 5 700
i 1 .03 .006 7 700
i 1 .04 .006 15 700
i 1 .05 .006 21 700
i 1 .06 .006 35 700
i 1 .07 .006 105 700
i 1 .08 .006 1 12000
i 1 .09 .006 3 12000
i 1 .10 .006 5 12000
i 1 .11 .006 7 12000
i 1 .12 .006 15 12000
i 1 .13 .006 21 12000
i 1 .14 .006 35 12000
i 1 .15 .006 105 12000
i 1 .16 .006 1 .00001
i 1 .17 .006 3 .00001
i 1 .18 .006 5 .00001
i 1 .19 .006 7 .00001
i 1 .20 .006 15 .00001
i 1 .21 .006 21 .00001
i 1 .22 .006 35 .00001
i 1 .23 .006 105 .00001
i 99 .24 .002
</CsScore>
</CsoundSynthesizer>
