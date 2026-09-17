<CsTest>
description = "lastcycle counts from the note start and supplies a minimum release"
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
  ; No explicit release: lastcycle must keep its one-cycle minimum.
  xtratim 0
  kAfter lastcycle

  ; Both placements must produce 0 on every cycle except the final one.
  kCycle timeinstk
  kExpected = (kCycle == 5 ? 1 : 0)
  if kBefore != kExpected || kAfter != kExpected then
    printks "delayed start: cycle %g, expected %g, before %g, after %g\n", 0, kCycle, kExpected, kBefore, kAfter
    exitnowk -1
  endif
  gkFinalCycle = kCycle
endin

instr 99
  ; Check after the note ends, so an early or late note end cannot pass.
  if i(gkFinalCycle) != 5 then
    prints "Expected the note to end on cycle %g, got %g\n", 5, i(gkFinalCycle)
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Start after time zero: four note cycles + one release cycle = five.
i1 .25 .0625
i99 .375 .001
e
</CsScore>
</CsoundSynthesizer>
