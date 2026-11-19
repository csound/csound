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

instr 2
  ; At DC and Nyquist the recurrence must not build a ramp from roundoff
  ; in sin(pi). Check a longer table, including a zero-phase silent partial.
  iSource ftgen 0, 0, -3, -2, 1, p4, p5
  iTable ftgen 0, 0, 4096, -34, iSource, 1, 1, p6
  iAmplitude = (p7 != 0 || p5 == 0 ? 0 : 1)
  iIndex = 0
  while iIndex < 4096 do
    iExpected = iAmplitude * (p4 == 0 ? 1 : 1 - 2*(iIndex % 2))
    iActual table iIndex, iTable
    if !(abs(iActual-iExpected) < .000001) then
      prints "GEN34 partial %g, phase %g, mode %g at %g: %g expected %g\n", p4, p5, p6, iIndex, iActual, iExpected
      exitnow -1
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
i 2 0 .01 2048 0 0 0
i 2 0 .01 -2048 0 0 0
i 2 0 .01 2048 .25 0 0
i 2 0 .01 -2048 .25 0 0
i 2 0 .01 0 .25 0 0
; 41 * (1/82) rounds below Nyquist in float builds.
i 2 0 .01 41 0 82 0
i 2 0 .01 -41 0 82 0
i 2 0 .01 41 .25 82 0
; The next float above 41 is outside the cutoff.
i 2 0 .01 41.000003814697265625 .25 82 1
i 2 0 .01 24000 .25 -1 0
</CsScore>
</CsoundSynthesizer>
