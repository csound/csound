<CsTest>
description = "tab2array clamps the slice end to the table length"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1

instr 1
  iTable ftgen 0, 0, -4, -2, 1, 2, 3, 4
  iValues[] tab2array iTable, 1, 100, 2
  kValues[] tab2array iTable, 1, 100, 2
  if lenarray(iValues) != 2 then
    exitnow -1
  endif
  if iValues[0] != 2 || iValues[1] != 4 then
    exitnow -1
  endif
  if lenarray(kValues) != 2 then
    exitnowk -1
  endif
  if kValues[0] != 2 || kValues[1] != 4 then
    exitnowk -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .015625
e
</CsScore>
</CsoundSynthesizer>
