<CsTest>
description = "vdelayk keeps integer write-head positions for zero delay"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1048576
ksmps = 1
nchnls = 1
0dbfs = 1

instr 1
 kInput init 0
 kInput = 1-kInput
 kOut vdelayk kInput, 0, 16+2/sr, 0, 1
 if kOut != kInput then
   printks "vdelayk read the wrong cell for zero delay\n", 0
   exitnowk -1
 endif
endin
</CsInstruments>
<CsScore>
i 1 0 16.001
</CsScore>
</CsoundSynthesizer>
