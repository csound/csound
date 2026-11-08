<CsTest>
description = "compile for-in loops with an empty or comment-only body"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m128
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 1
nchnls = 1

instr 1
  for v in [1] do
  od
endin

instr 2
  for v in [1, 2, 3] do
    /* only a comment */
  od
endin

instr 3
  for i, v in [1, 2] do
  od
endin

instr 4
  karr[] fillarray 1, 2, 3
  for v in karr do
  od
endin

instr 5
  icount = 0
  for v in [1, 2, 3] do
    icount += 1
  od
  if (icount != 3) then
    prints "for-in loop iterated %d times, expected 3\n", icount
    exitnow(1)
  endif
endin
</CsInstruments>
<CsScore>
i1 0 0
i2 0 0
i3 0 0
i4 0 0
i5 0 0
e
</CsScore>
</CsoundSynthesizer>
