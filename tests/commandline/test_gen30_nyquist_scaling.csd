<CsTest>
description = "GEN30 preserves harmonic amplitudes when table sizes change"

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
    iPhase = 2 * $M_PI * iIndex / iLength
    iExpected = .25 + .5*cos(iPhase) + p5*cos(4*iPhase)
    iActual table iIndex, p4
    if !(abs(iActual - iExpected) < .00001) then
      prints "GEN30 table %g at index %g: %g, expected %g\n", p4, iIndex, iActual, iExpected
      exitnow(-1)
    endif
    iIndex += 1
  od
endin
</CsInstruments>
<CsScore>
; DC, fundamental, and fourth harmonic; disable post-normalization.
f 1 0 8 -9 0 .25 90 1 .5 90 4 1 90
f 2 0 16 -9 0 .25 90 1 .5 90 4 1 90
f 3 0 16 -30 1 0 4
f 4 0 8 -30 2 0 4
f 5 0 8 -30 1 0 4
; A fractional cutoff keeps half of the fourth harmonic when enlarging.
f 6 0 16 -30 1 0 3.5 0 1
i 1 0 .01 3 1
i 1 0 .01 4 1
i 1 0 .01 5 1
i 1 0 .01 6 .5
</CsScore>
</CsoundSynthesizer>
