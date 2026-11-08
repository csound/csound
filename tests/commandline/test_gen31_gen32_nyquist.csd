<CsTest>
description = "GEN31 and GEN32 retain Nyquist partials with correct amplitude and phase"

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
    iPhase = 2 * $M_PI * (p5 * iIndex / iLength + p6)
    iExpected = .25 + .5*cos(iPhase) + cos(4*iPhase)
    iActual table iIndex, p4
    if !(abs(iActual - iExpected) < .00001) then
      prints "Table %g at index %g: %g, expected %g\n", p4, iIndex, iActual, iExpected
      exitnow(-1)
    endif
    iIndex += 1
  od
endin
</CsInstruments>
<CsScore>
; DC, fundamental and fourth harmonic, without post-normalization.
f 1 0 8 -9 0 .25 90 1 .5 90 4 1 90
f 2 0 16 -9 0 .25 90 1 .5 90 4 1 90
; Unity copy, enlargement with phase shift, reduction, and transposition.
f 3 0 8 -31 1 1 1 0
f 4 0 16 -31 1 1 1 .0625
f 5 0 8 -31 2 1 1 0
f 6 0 16 -31 1 2 1 .125
f 7 0 16 -32 1 1 1 .0625
f 8 0 8 -32 2 1 1 0
f 9 0 16 -32 1 2 1 .125
i 1 0 .01 3 1 0
i 1 0 .01 4 1 .0625
i 1 0 .01 5 1 0
i 1 0 .01 6 2 .125
i 1 0 .01 7 1 .0625
i 1 0 .01 8 1 0
i 1 0 .01 9 2 .125
</CsScore>
</CsoundSynthesizer>
