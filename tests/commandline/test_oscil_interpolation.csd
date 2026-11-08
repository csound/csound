<CsTest>
description = "non-power-of-two tables retain the requested oscillator interpolation"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 8
nchnls = 1
0dbfs = 1
giWave ftgen 1,0,-5,-2,0,1,0,0,0
instr 1
  ; Halfway between the first two entries: 0, .5, and .5625.
  aNone oscil 1,0,giWave,.1
  aLinear oscili 1,0,giWave,.1
  aCubic oscil3 1,0,giWave,.1
  kNone oscil 1,0,giWave,.1
  kLinear oscili 1,0,giWave,.1
  kCubic oscil3 1,0,giWave,.1
  kANone downsamp aNone
  kALinear downsamp aLinear
  kACubic downsamp aCubic
  if abs(kANone) > .000001 || abs(kALinear-.5) > .000001 || abs(kACubic-.5625) > .000001 || abs(kNone) > .000001 || abs(kLinear-.5) > .000001 || abs(kCubic-.5625) > .000001 then
    printks "oscillator selected the wrong interpolation\n",0
    exitnowk(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .001
e
</CsScore>
</CsoundSynthesizer>
