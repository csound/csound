<CsTest>
description = "lastcycle observes a p3 extension made later during initialization"
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
  xtratim .0625
  ; Extend p3 from four to eight cycles after the first lastcycle initializes.
  p3 += .0625
  kAfter lastcycle

  ; Both placements must produce 0 on every cycle except the final one.
  kCycle timeinstk
  kExpected = (kCycle == 12 ? 1 : 0)
  if kBefore != kExpected || kAfter != kExpected then
    printks "p3 extension: cycle %g, expected %g, before %g, after %g\n", 0, kCycle, kExpected, kBefore, kAfter
    exitnowk -1
  endif
  gkFinalCycle = kCycle
endin

instr 99
  ; Check after the note ends, so an early or late note end cannot pass.
  if i(gkFinalCycle) != 12 then
    prints "Expected the note to end on cycle %g, got %g\n", 12, i(gkFinalCycle)
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Eight note cycles + four release cycles = twelve cycles.
i1 .5 .0625
i99 .75 .001
e
</CsScore>
</CsoundSynthesizer>
