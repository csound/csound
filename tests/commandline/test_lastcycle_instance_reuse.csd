<CsTest>
description = "lastcycle resets its state when an instance is reused with a shorter or no release"
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
  iRelease = p4
  iExpectedCycles = p5
  kBefore lastcycle
  xtratim iRelease
  kAfter lastcycle

  ; Both placements must produce 0 on every cycle except the final one.
  ; Cycle indices start at zero.
  kCycleIndex eventcycles
  kExpected = (kCycleIndex == iExpectedCycles - 1 ? 1 : 0)
  if kBefore != kExpected || kAfter != kExpected then
    printks "instance reuse: cycle index %g, expected pulse %g, before %g, after %g\n", 0, kCycleIndex, kExpected, kBefore, kAfter
    exitnowk -1
  endif
  ; Include the current cycle in the total.
  gkCyclesRendered = kCycleIndex + 1
endin

instr 99
  ; Check after the note ends, so an early or late note end cannot pass.
  if i(gkCyclesRendered) != p4 then
    prints "Expected %g rendered cycles, got %g\n", p4, i(gkCyclesRendered)
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Each note finishes before the next, allowing instance reuse.
;     start   p3       release  expected total cycles
i1    0       .0625   .0625    8
i99   .1875   .001    8
i1    .25     .0625   .03125   6
i99   .375    .001    6
i1    .5      .0625   0        4
i99   .625    .001    4
e
</CsScore>
</CsoundSynthesizer>
