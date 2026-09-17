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
gkFinalCycle init 0

instr 1
  kBefore lastcycle
  xtratim .0625
  kAfter lastcycle

  ; Both placements must produce 0 on every cycle except the final one.
  kCycle timeinstk
  kExpected = (kCycle == 9 ? 1 : 0)
  if kBefore != kExpected || kAfter != kExpected then
    printks "indefinite turnoff: cycle %g, expected %g, before %g, after %g\n", 0, kCycle, kExpected, kBefore, kAfter
    exitnowk -1
  endif
  gkFinalCycle = kCycle
endin

instr 2
  ; Stop only this fractional instance and allow its release stage.
  turnoff2 1.03, 4, 1
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
; Instrument 2 runs after instrument 1 on cycle five.
; Five cycles through the stop request + four release cycles = nine.
i1.03 .75 -1
i2 .8125 .01
i99 .9375 .001
e
</CsScore>
</CsoundSynthesizer>
