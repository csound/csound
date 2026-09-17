<CsTest>
description = "lastcycle resets its state when an instance is reused with a shorter release"
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
  iRelease = p4
  iLastCycle = p5
  kBefore lastcycle
  xtratim iRelease
  kAfter lastcycle

  ; Both placements must produce 0 on every cycle except the final one.
  kCycle timeinstk
  kExpected = (kCycle == iLastCycle ? 1 : 0)
  if kBefore != kExpected || kAfter != kExpected then
    printks "instance reuse: cycle %g, expected %g, before %g, after %g\n", 0, kCycle, kExpected, kBefore, kAfter
    exitnowk -1
  endif
  gkFinalCycle = kCycle
endin

instr 99
  ; Check after the note ends, so an early or late note end cannot pass.
  if i(gkFinalCycle) != p4 then
    prints "Expected the note to end on cycle %g, got %g\n", p4, i(gkFinalCycle)
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; The first note finishes before the second, allowing instance reuse.
;     start   p3       release  expected final cycle
i1    0       .0625   .0625    8
i99   .1875   .001    8
i1    .25     .0625   .03125   6
i99   .375    .001    6
e
</CsScore>
</CsoundSynthesizer>
