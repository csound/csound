<CsTest>
description = "lastcycle observes a later p3 extension without adding a release"
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
gkCyclesRendered init 0

instr 1
  kBefore lastcycle
  ; Extend p3 from four to eight cycles after the first lastcycle initializes.
  p3 += .0625
  kAfter lastcycle

  ; Both placements must produce 0 on every cycle except the final one.
  ; Cycle indices start at zero.
  kCycleIndex eventcycles
  kExpected = (kCycleIndex == 7 ? 1 : 0)
  if kBefore != kExpected || kAfter != kExpected then
    printks "p3 extension without release: cycle index %g, expected pulse %g, before %g, after %g\n", 0, kCycleIndex, kExpected, kBefore, kAfter
    exitnowk -1
  endif
  ; Include the current cycle in the total.
  gkCyclesRendered = kCycleIndex + 1
endin

instr 99
  ; Check after the note ends, so an early or late note end cannot pass.
  if i(gkCyclesRendered) != 8 then
    prints "Expected %g rendered cycles, got %g\n", 8, i(gkCyclesRendered)
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; The later p3 change gives eight cycles, with no release extension.
i1 .5 .0625
i99 .75 .001
e
</CsScore>
</CsoundSynthesizer>
