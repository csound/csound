<CsTest>
description = "GEN27 honors breakpoint positions and the final endpoint"

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
  iMiddle table 8, 1
  ; This lookup includes the extended guard point at index 16.
  iEnd tablei 15.5, 1
  if !(abs(iMiddle - .5) < .00001 && abs(iEnd - .96875) < .00001) then
    prints "GEN27 ramp: midpoint %g, final interpolation %g\n", iMiddle, iEnd
    exitnow(-1)
  endif
endin

instr 2
  iBefore table 3, p4
  iStart table 4, p4
  iMiddle table 6, p4
  iEnd table 8, p4
  iAfter table 9, p4
  if !(iBefore == 0 && abs(iStart - 1) < .00001 && abs(iMiddle - 2) < .00001 && abs(iEnd - 3) < .00001 && iAfter == 0) then
    prints "GEN27 table %g: %g %g %g %g %g\n", p4, iBefore, iStart, iMiddle, iEnd, iAfter
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
f 1 0 17 -27 0 0 16 1
; Leading and trailing samples stay zero around these breakpoints.
f 2 0 17 -27 4 1 6 2 8 3
; Fractional coordinates describe the same line at integer samples.
f 3 0 17 -27 3.5 .75 6.5 2.25 8 3
i 1 0 .01
i 2 0 .01 2
i 2 0 .01 3
</CsScore>
</CsoundSynthesizer>
