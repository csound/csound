<CsTest>
description = "lastcycle follows the final note and release durations"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
gkCycles[] init 6
gkBefore[] init 6
gkAfter[] init 6
gkBeforeHits[] init 6
gkAfterHits[] init 6

instr 1
  iSlot = p6
  kBefore lastcycle
  xtratim p4
  if p5 != 0 then
    p3 += .0625
  endif
  kAfter lastcycle
  kCycle timeinstk
  gkCycles[iSlot] = kCycle
  if kBefore == 1 then
    gkBefore[iSlot] = kCycle
    gkBeforeHits[iSlot] += 1
  endif
  if kAfter == 1 then
    gkAfter[iSlot] = kCycle
    gkAfterHits[iSlot] += 1
  endif
endin

instr 2
  turnoff2 p4, 4, 1
  turnoff
endin

instr 99
  kIndex = 0
  while kIndex < 6 do
    if gkCycles[kIndex] == 0 || gkBefore[kIndex] != gkCycles[kIndex] || gkAfter[kIndex] != gkCycles[kIndex] || gkBeforeHits[kIndex] != 1 || gkAfterHits[kIndex] != 1 then
      printks "lastcycle case %d: final %g, before %g, after %g, hits %g/%g\n", 0, kIndex, gkCycles[kIndex], gkBefore[kIndex], gkAfter[kIndex], gkBeforeHits[kIndex], gkAfterHits[kIndex]
      exitnowk -1
    endif
    kIndex += 1
  od
  turnoff
endin
</CsInstruments>
<CsScore>
; Release extension, delayed start without an extension, and a later p3 change.
i1 0 .0625 .0625 0 0
i1 .25 .0625 0 0 1
i1 .5 .0625 .0625 1 2
; Indefinite and finite notes stopped early, allowing their release stages.
i1.03 .75 -1 .0625 0 3
i2 .8125 .01 1.03
i1.04 1 10 .0625 0 4
i2 1.0625 .01 1.04
; Reuse a finished instance with a different release length.
i1 1.25 .0625 .03125 0 5
i99 1.5 .01
</CsScore>
</CsoundSynthesizer>
