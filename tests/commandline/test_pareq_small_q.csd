<CsTest>
description = "pareq normalizes near DC before dividing by a tiny Q"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --preserve-subnormals
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 1
nchnls = 1
0dbfs = 1

instr 1
  aIn = 1
  aOut pareq aIn, 1e-25, 2, 1e-39, 0
  kOut downsamp aOut
  if !(abs(kOut - 2) < .00001) then
    printks "pareq overflowed while normalizing the band coefficient\n", 0
    exitnowk -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .008 55
</CsScore>
</CsoundSynthesizer>
