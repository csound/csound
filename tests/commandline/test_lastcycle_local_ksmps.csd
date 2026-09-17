<CsTest>
description = "lastcycle follows a sample-accurate end with a smaller local ksmps"
[expect]
exit = 0
</CsTest>

<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
; Global blocks hold 16 samples; instrument 1 uses four-sample local blocks.
sr = 1024
ksmps = 16
nchnls = 1
gkCyclesRendered init 0

instr 1
  setksmps 4
  kBefore lastcycle
  ; No release: neither lastcycle call may extend this note.
  xtratim 0
  kAfter lastcycle

  ; Both placements must produce 0 on every cycle except the final one.
  ; Cycle indices start at zero.
  kCycleIndex eventcycles
  kExpected = (kCycleIndex == 17 ? 1 : 0)
  if kBefore != kExpected || kAfter != kExpected then
    printks "local ksmps: cycle index %g, expected pulse %g, before %g, after %g\n", 0, kCycleIndex, kExpected, kBefore, kAfter
    exitnowk -1
  endif
  ; Include the current cycle in the total.
  gkCyclesRendered = kCycleIndex + 1
endin

instr 99
  ; Check after the note ends, so an early or late note end cannot pass.
  if i(gkCyclesRendered) != 18 then
    prints "Expected %g rendered cycles, got %g\n", 18, i(gkCyclesRendered)
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; 72 samples occupy 18 local blocks. The unused end of the global block
; must not postpone the pulse beyond the final local cycle (index 17).
i1 0 .0703125
i99 .125 .001
e
</CsScore>
</CsoundSynthesizer>
