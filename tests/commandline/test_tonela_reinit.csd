<CsTest>
description = "tonela and atonela preserve or clear state on reinitialization"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
  kCycle init 0
  kCycle += 1
  kCutoff = kCycle < 5 ? 700 : 12000
  aInput oscili .5, 321
  if kCycle == 5 then
    reinit FILTER
  endif
FILTER:
  iSkip = i(kCycle) > 4 ? p4 : 0
  aLow tonela aInput, kCutoff, iSkip
  aHigh atonela aInput, kCutoff, iSkip
  aRefLow tone aInput, kCutoff, iSkip
  aRefHigh atone aInput, kCutoff, iSkip
  rireturn
  aError = abs(aLow-aRefLow) + abs(aHigh-aRefHigh)
  kError max_k aError, 1, 1
  if !(kError <= .000001) then
    printks "tonela reinit iskip=%g mismatch: %g\n", 0, p4, kError
    exitnowk(-1)
  endif
  kFirst init 1
  if kFirst == 1 then
    gkChecks += 1
    kFirst = 0
  endif
endin

instr 99
  if i(gkChecks) != 2 then
    prints "tonela reinit completed %g checks; expected 2\n", i(gkChecks)
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .02 0
i 1 .03 .02 1
i 99 .06 .002
</CsScore>
</CsoundSynthesizer>
