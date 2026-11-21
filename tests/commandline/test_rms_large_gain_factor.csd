<CsTest>
description = "level matching keeps finite output with a large gain factor"

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
  aInput = 2^(-130)
  aRef = 1
  aGained gain aInput, 1, 100
  aBalanced balance2 aInput, aRef, 100
  kGain downsamp aGained
  kBalance downsamp aBalanced
  if !(abs(kBalance - 1) < .00001 && kGain >= 0 && kGain < 2) then
    printks "level matching overflowed the gain factor\n", 0
    exitnowk -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .008 55
</CsScore>
</CsoundSynthesizer>
