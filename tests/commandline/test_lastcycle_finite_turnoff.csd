<CsTest>
description = "lastcycle follows an early turnoff instead of the original finite duration"
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
  xtratim .0625
  kAfter lastcycle

  ; Both placements must produce 0 on every cycle except the final one.
  ; Cycle indices start at zero.
  kCycleIndex eventcycles
  kExpected = (kCycleIndex == 8 ? 1 : 0)
  if kBefore != kExpected || kAfter != kExpected then
    printks "finite turnoff: cycle index %g, expected pulse %g, before %g, after %g\n", 0, kCycleIndex, kExpected, kBefore, kAfter
    exitnowk -1
  endif
  ; Include the current cycle in the total.
  gkCyclesRendered = kCycleIndex + 1
endin

instr 2
  ; Stop only this fractional instance and allow its release stage.
  turnoff2 1.04, 4, 1
  turnoff
endin

instr 99
  ; Check after the note ends, so an early or late note end cannot pass.
  if i(gkCyclesRendered) != 9 then
    prints "Expected %g rendered cycles, got %g\n", 9, i(gkCyclesRendered)
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Stop the ten-second note on the fifth cycle (index 4), well before its scheduled end.
; Five cycles through the stop request + four release cycles = nine.
i1.04 1 10
i2 1.0625 .01
i99 1.1875 .001
e
</CsScore>
</CsoundSynthesizer>
