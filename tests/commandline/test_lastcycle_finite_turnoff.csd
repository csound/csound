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
gkFinalCycle init 0

instr 1
  kBefore lastcycle
  xtratim .0625
  kAfter lastcycle

  ; Both placements must produce 0 on every cycle except the final one.
  kCycle timeinstk
  kExpected = (kCycle == 9 ? 1 : 0)
  if kBefore != kExpected || kAfter != kExpected then
    printks "finite turnoff: cycle %g, expected %g, before %g, after %g\n", 0, kCycle, kExpected, kBefore, kAfter
    exitnowk -1
  endif
  gkFinalCycle = kCycle
endin

instr 2
  ; Stop only this fractional instance and allow its release stage.
  turnoff2 1.04, 4, 1
  turnoff
endin

instr 99
  ; Check after the note ends, so an early or late note end cannot pass.
  if i(gkFinalCycle) != 9 then
    prints "Expected the note to end on cycle %g, got %g\n", 9, i(gkFinalCycle)
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Stop the ten-second note on cycle five, well before its scheduled end.
; Five cycles through the stop request + four release cycles = nine.
i1.04 1 10
i2 1.0625 .01
i99 1.1875 .001
e
</CsScore>
</CsoundSynthesizer>
