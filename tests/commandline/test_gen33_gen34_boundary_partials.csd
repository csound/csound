<CsTest>
description = "GEN33 and GEN34 preserve DC and Nyquist amplitudes and phases"

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
  ; Mix DC, a negative fundamental, and both signs of Nyquist.
  iSource ftgen 0, 0, -12, -2, .25, 0, p5, .5, -1, .125, 1, 4, p5, .25, -4, .25
  iTable ftgen 0, 0, 8, -p4, iSource, 4, 1
  iIndex = 0
  while iIndex < 8 do
    iTime = iIndex / 8
    iExpected = .25*sin(2*$M_PI*p5) + .5*sin(2*$M_PI*(-iTime+.125))
    iExpected += sin(2*$M_PI*(4*iTime+p5)) + .25*sin(2*$M_PI*(-4*iTime+.25))
    iActual table iIndex, iTable
    if !(abs(iActual - iExpected) < .00001) then
      prints "GEN%g phase %g at index %g: %g, expected %g\n", p4, p5, iIndex, iActual, iExpected
      exitnow(-1)
    endif
    iIndex += 1
  od
endin
</CsInstruments>
<CsScore>
i 1 0 .01 33 0
i 1 0 .01 33 .25
i 1 0 .01 34 0
i 1 0 .01 34 .25
</CsScore>
</CsoundSynthesizer>
