<CsTest>
description = "GEN25 honors breakpoint positions, endpoints, and negative values"

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
  iBefore table 3, p4
  iStart table 4, p4
  iMiddle table 6, p4
  iEnd table 8, p4
  iAfter table 9, p4
  if !(iBefore == 0 && abs(iStart - p5) < .00001 && abs(iMiddle - 2*p5) < .00001 && abs(iEnd - 4*p5) < .00001 && iAfter == 0) then
    prints "GEN25 table %g: %g %g %g %g %g\n", p4, iBefore, iStart, iMiddle, iEnd, iAfter
    exitnow(-1)
  endif
endin

instr 2
  iStart table 0, 4
  iMiddle table 8, 4
  iEnd tablei 15.5, 4
  iExpected = (pow(4, 15/16) + 4) / 2
  if !(iStart == 1 && abs(iMiddle - 2) < .00001 && abs(iEnd - iExpected) < .00001) then
    prints "GEN25 full-table curve or guard point is incorrect\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
f 1 0 17 -25 4 1 6 2 8 4
f 2 0 17 -25 4 -1 6 -2 8 -4
; Fractional breakpoints lie on the same exponential curve.
f 3 0 17 -25 3.5 .8408964152537145 6.5 2.378414230005442 8 4
f 4 0 17 -25 0 1 16 4
i 1 0 .01 1 1
i 1 0 .01 2 -1
i 1 0 .01 3 1
i 2 0 .01
</CsScore>
</CsoundSynthesizer>
