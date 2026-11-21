<CsTest>
description = "tonela uses post-offset history when the input before the offset is nonzero"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
  gaInput oscili .5, 321
endin

instr 2
  aRefLow tone gaInput, 700
  aRefHigh atone gaInput, 700
  aLow tonela gaInput, 700
  aHigh atonela gaInput, 700
  aError = abs(aLow-aRefLow) + abs(aHigh-aRefHigh)
  kError max_k aError, 1, 1
  if !(kError <= .000001) then
    printks "tonela offset history mismatch: %g\n", 0, kError
    exitnowk(-1)
  endif
  kFirst init 1
  if kFirst == 1 then
    gkChecks += 1
    kFirst = 0
  endif
endin

instr 99
  if i(gkChecks) != 3 then
    prints "tonela offset history completed %g checks; expected 3\n", i(gkChecks)
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .05
i 2 [3/48000] .01
i 2 [5/48000] .01
i 2 [19/48000] .01
i 99 .06 .002
</CsScore>
</CsoundSynthesizer>
