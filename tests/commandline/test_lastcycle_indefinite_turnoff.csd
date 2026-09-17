<CsTest>
description = "lastcycle fires at the end of an indefinite note stopped with release"
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
    printks "indefinite turnoff: cycle index %g, expected pulse %g, before %g, after %g\n", 0, kCycleIndex, kExpected, kBefore, kAfter
    exitnowk -1
  endif
  ; Include the current cycle in the total.
  gkCyclesRendered = kCycleIndex + 1
endin

instr 2
  ; Stop only this fractional instance and allow its release stage.
  turnoff2 1.03, 4, 1
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
; Instrument 2 runs after instrument 1 on the fifth cycle (index 4).
; Five cycles through the stop request + four release cycles = nine.
i1.03 .75 -1
i2 .8125 .01
i99 .9375 .001
e
</CsScore>
</CsoundSynthesizer>
