<CsTest>
description = "tonela and atonela handle cutoff changes, input reuse, and sample offsets"

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
  kCycle init 0
  kCycle += 1
  kCutoff = kCycle < 3 ? 500 : (kCycle < 5 ? 12000 : 700)
  aInput oscili .5, 321
  aRefLow tone aInput, kCutoff
  aRefHigh atone aInput, kCutoff
  aLow tonela aInput, kCutoff
  aHigh atonela aInput, kCutoff
  aAliasLowIn = aInput
  aAliasHighIn = aInput
  aAliasLow tonela aAliasLowIn, kCutoff
  aAliasHigh atonela aAliasHighIn, kCutoff
  aError = abs(aLow-aRefLow) + abs(aHigh-aRefHigh)
  aError += abs(aAliasLow-aRefLow) + abs(aAliasHigh-aRefHigh)
  kError max_k aError, 1, 1
  if !(kError <= .000001) then
    printks "tonela update/alias mismatch: error=%g\n", 0, kError
    exitnowk(-1)
  endif
  kFirst init 1
  if kFirst == 1 then
    gkChecks += 1
    kFirst = 0
  endif
endin

instr 99
  if i(gkChecks) != 4 then
    prints "tonela update/alias completed %g checks; expected 4\n", i(gkChecks)
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 [1/48000] .01
i 1 [2/48000] .01
i 1 [3/48000] .01
i 1 [17/48000] .01
i 99 .02 .002
</CsScore>
</CsoundSynthesizer>
