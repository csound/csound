<CsTest>
description = "Slow expsegr curves advance with float samples"
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

instr SlowCurve
  ; A gradual rise from 1 to 1.05 must not round its multiplier to 1.
  kLine expsegr 1, 32, 1.05, .001, 1
  aLine expsegr 1, 32, 1.05, .001, 1
  kAudio downsamp aLine
  kStep timeinstk
  ; At 16 seconds the exponential is at the geometric mean.
  if kStep-1 == 16*kr then
    kExpected = sqrt(1.05)
    if !(abs(kLine-kExpected) < .000001 && abs(kAudio-kExpected) < .000001) then
      printks "Slow curve: expected %g, got control=%g audio=%g\n", 0, kExpected, kLine, kAudio
      exitnowk -1
    endif
  endif
endin
</CsInstruments>
<CsScore>
i "SlowCurve" 0 16.01
e
</CsScore>
</CsoundSynthesizer>
