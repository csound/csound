<CsTest>
description = "phaser2 scales large low-frequency histories"

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

instr 1
  aInput = 2e38
  aOut phaser2 aInput, .000001, 1, 1, 1, 1, 0
  kOut downsamp aOut
  if !(abs(kOut/2e38 - 1) < .00001) then
    printks "phaser2 low-frequency state overflowed\n", 0
    exitnowk -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .008 55
</CsScore>
</CsoundSynthesizer>
