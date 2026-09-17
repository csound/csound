<CsTest>
description = "lastcycle works before and after a later xtratim"
[expect]
exit = 0
</CsTest>

<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
; One control cycle is 16/1024 = 0.015625 seconds.
sr = 1024
ksmps = 16
nchnls = 1
gkFinalCycle init 0

instr 1
  kBefore lastcycle
  ; Four release cycles added after the first lastcycle initializes.
  xtratim .0625
  kAfter lastcycle

  ; Both placements must produce 0 on every cycle except the final one.
  kCycle timeinstk
  kExpected = (kCycle == 8 ? 1 : 0)
  if kBefore != kExpected || kAfter != kExpected then
    printks "xtratim order: cycle %g, expected %g, before %g, after %g\n", 0, kCycle, kExpected, kBefore, kAfter
    exitnowk -1
  endif
  gkFinalCycle = kCycle
endin

instr 99
  ; Check after the note ends, so an early or late note end cannot pass.
  if i(gkFinalCycle) != 8 then
    prints "Expected the note to end on cycle %g, got %g\n", 8, i(gkFinalCycle)
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Four note cycles + four release cycles = eight cycles.
i1 0 .0625
i99 .1875 .001
e
</CsScore>
</CsoundSynthesizer>
