<CsTest>
description = "scale2 keeps the distance to the upper input endpoint"

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
  kOut scale2 1-2^(-24), 1, 0, -1, 1
  if !(kOut == 2^(-25)) then
    printks "scale2 rounded to the input endpoint\n", 0
    exitnowk -1
  endif
  turnoff
endin
</CsInstruments>
<CsScore>
i 1 0 .008 55
</CsScore>
</CsoundSynthesizer>
