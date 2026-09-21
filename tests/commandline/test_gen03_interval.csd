<CsTest>
description = "GEN03 evaluates the polynomial over the requested interval"

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
  iStart table 0, p4
  iMiddle table 8, p4
  ; Interpolation here also checks the extended guard point.
  iEnd tablei 15.5, p4
  if !(abs(iStart - p5) < .00001 && abs(iMiddle - p6) < .00001 && abs(iEnd - p7) < .00001) then
    prints "Table %g: start %g, midpoint %g, final interpolation %g\n", p4, iStart, iMiddle, iEnd
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; Identity polynomials with starts between multiples of the sample spacing.
f 1 0 17 -3 .1 1.1 0 1
f 2 0 17 -3 -1.1 -.1 0 1
i 1 0 .01 1 .1 .6 1.06875
i 1 0 .01 2 -1.1 -.6 -.13125
; Preserve symmetric intervals and the GEN13/14 callers of GEN03.
f 3 0 17 -3 -1 1 0 1
f 4 0 17 -13 1 1 0 1
f 5 0 17 -14 1 1 0 .5
i 1 0 .01 3 -1 0 .9375
i 1 0 .01 4 -1 0 .9375
i 1 0 .01 5 -1 0 .9375
</CsScore>
</CsoundSynthesizer>
