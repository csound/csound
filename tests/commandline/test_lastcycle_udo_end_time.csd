<CsTest>
description = "lastcycle in nested UDOs follows later duration and release changes"
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

opcode FinalCycle, k, 0
  kLast lastcycle
  xout kLast
endop

opcode WrappedFinalCycle, k, 0
  kLast FinalCycle
  xout kLast
endop

instr 1
  kBefore WrappedFinalCycle
  ; Extend p3 from four to eight cycles after the first lastcycle initializes.
  p3 += .0625
  xtratim p4
  kAfter WrappedFinalCycle

  ; Both placements must produce 0 on every cycle except the final one.
  ; Cycle indices start at zero.
  kCycleIndex eventcycles
  kExpected = (kCycleIndex == p5 - 1 ? 1 : 0)
  if kBefore != kExpected || kAfter != kExpected then
    printks "nested UDO: cycle index %g, expected pulse %g, before %g, after %g\n", 0, kCycleIndex, kExpected, kBefore, kAfter
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
; The later p3 change gives eight cycles, with no release extension.
i1 .5 .0625 0 8
i99 .75 .001 8
; Repeat with four release cycles added after the first UDO initializes.
i1 1 .0625 .0625 12
i99 1.25 .001 12
e
</CsScore>
</CsoundSynthesizer>
