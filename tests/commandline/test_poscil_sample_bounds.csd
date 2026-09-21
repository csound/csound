<CsTest>
description = "poscil and oscillator fallback paths clear inactive samples"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 8
nchnls = 1
0dbfs = 1
giWave ftgen 1,0,-5,-2,1,1,1,1,1
instr 1
  aPos poscil .5,0,giWave
  aNone oscil .5,0,giWave
  aLinear oscili .5,0,giWave
  kBlock init 0
  kIndex = 0
  while kIndex < ksmps do
    kSample = kBlock*ksmps+kIndex
    kExpected = (kSample >= 3 && kSample < 14 ? .5 : 0)
    kPos vaget kIndex,aPos
    kNone vaget kIndex,aNone
    kLinear vaget kIndex,aLinear
    if kPos != kExpected || kNone != kExpected || kLinear != kExpected then
      printks "oscillator sample %g: expected %g, got %g %g %g\n",0,kSample,kExpected,kPos,kNone,kLinear
      exitnowk(-1)
    endif
    kIndex += 1
  od
  kBlock += 1
endin
</CsInstruments>
<CsScore>
i 1 .000375 .001375
e
</CsScore>
</CsoundSynthesizer>
