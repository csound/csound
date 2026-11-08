<CsTest>
description = "GEN06 truncated tables retain the curve through their guard point"

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
  iIndex = 0
  while iIndex < iLength do
    iActual table iIndex, p4
    iExpected table iIndex, p5
    if !(abs(iActual - iExpected) < .00001) then
      prints "GEN06 table %g differs from its full curve at %g\n", p4, iIndex
      exitnow(-1)
    endif
    iIndex += 1
  od
  ; Recover the extended guard point through interpolation.
  iLast table iLength-1, p4
  iBetween tablei iLength-.5, p4
  iExpected table iLength, p5
  if !(abs(2*iBetween-iLast-iExpected) < .00001) then
    prints "GEN06 table %g endpoint: %g, expected %g\n", p4, 2*iBetween-iLast, iExpected
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; Truncate within the first segment and exactly at its endpoint.
f 1 0 9 -6 0 16 1 16 0
f 2 0 17 -6 0 16 1 16 0
f 3 0 33 -6 0 16 1 16 0
; Truncate within the second segment, which runs towards an extremum.
f 4 0 17 -6 0 8 1 24 0
f 5 0 33 -6 0 8 1 24 0
i 1 0 .01 1 3
i 1 0 .01 2 3
i 1 0 .01 4 5
</CsScore>
</CsoundSynthesizer>
