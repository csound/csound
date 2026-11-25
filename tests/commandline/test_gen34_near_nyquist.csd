<CsTest>
description = "GEN34 retains the sinusoidal shape just below Nyquist"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 1
nchnls = 1
0dbfs = 1

giSource ftgen 1, 0, -3, -2, 1, 2047.9, 0
giDest ftgen 2, 0, 4096, -34, giSource, 1, 1
instr 1
  iOut table 4095, giDest
  iFrequency table 1, giSource
  iExpected = sin(2*$M_PI*(2048-iFrequency)*4095/4096)
  if !(abs(iOut - iExpected) < .0003) then
    prints "GEN34 near-Nyquist oscillator drifted\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .008 55
</CsScore>
</CsoundSynthesizer>
