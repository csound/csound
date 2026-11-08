<CsTest>
description = "scanu and scanu2 reject output tables shorter than the network"
[expect]
exit = "nonzero"
stderr = ["scanu: output table is too short"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 1
nchnls = 1
instr 1
  iZero ftgen 0, 0, 8, -2, 0
  iMass ftgen 0, 0, 8, -7, 1, 8, 1
  iMatrix ftgen 0, 0, 64, -2, 0
  iOutput ftgen 0, 0, 4, -2, 0
  aDrive = 0
  if p4 == 0 then
    scanu iZero, .01, iZero, iMass, iMatrix, iZero, iZero, 1, 1, 0, 0, 0, 0, 0, 0, aDrive, 0, -iOutput
  else
    scanu2 iZero, .01, iZero, iMass, iMatrix, iZero, iZero, 1, 1, 0, 0, 0, 0, 0, 0, aDrive, 0, -iOutput
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .01 0
i 1 0 .01 1
e
</CsScore>
</CsoundSynthesizer>
