<CsTest>
description = "GEN11 normalizes geometric weights that would overflow"

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

giTable ftgen 1, 0, -32, -11, 200, 1, 2
instr 1
  iFirst table 0, giTable
  iNext table 1, giTable
  if !(iFirst == 1 && abs(iNext - .181166) < .00001) then
    prints "GEN11 overflowed its geometric weights\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .008 55
</CsScore>
</CsoundSynthesizer>
