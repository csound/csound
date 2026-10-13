<CsTest>
description = "GEN16 fills negative-sized tables and preserves guard-point modes"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

instr 1
  iLength = ftlen(p4)
  iMiddle table iLength / 2, p4
  ; Interpolation at the last half-sample includes the guard point.
  iEnd tablei iLength - .5, p4
  if !(abs(iMiddle - .5) < .00001 && abs(iEnd - p5) < .00001) then
    prints "GEN16 table %g: midpoint %g, final interpolation %g\n", p4, iMiddle, iEnd
    exitnow(-1)
  endif
endin

instr 2
  iMiddle table 8, 5
  if !(abs(iMiddle - 1/3) < .00001) then
    prints "GEN16 curved midpoint: %g\n", iMiddle
    exitnow(-1)
  endif
  iIndex = 0
  while iIndex < 16 do
    iPositive tablei iIndex, 5
    iNegative tablei iIndex, 6
    if !(abs(iPositive - iNegative) < .00001) then
      prints "GEN16 curved tables differ at %g\n", iIndex
      exitnow(-1)
    endif
    iIndex += .5
  od
endin
</CsInstruments>
<CsScore>
f 1 0 16 -16 0 16 0 1
f 2 0 17 -16 0 16 0 1
f 3 0 -16 -16 0 16 0 1
f 4 0 -10 -16 0 10 0 1
; Curve type ln(4) gives a midpoint of 1/3.
f 5 0 17 -16 0 16 1.3862943611198906 1
f 6 0 -16 -16 0 16 1.3862943611198906 1
i 1 0 .01 1 .46875
i 1 0 .01 2 .96875
i 1 0 .01 3 .96875
i 1 0 .01 4 .95
i 2 0 .01
</CsScore>
</CsoundSynthesizer>
