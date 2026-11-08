<CsTest>
description = "tabmorph rejects empty, oversized, and multidimensional table arrays"
[expect]
exit = "nonzero"
stderr = ["tabmorph: table count out of range", "tabmorph: expected a one-dimensional table array"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
giTable ftgen 0, 0, 4, -2, 1
instr Count
  iTables[] init p4
  iIndex = 0
  while iIndex < p4 do
    iTables[iIndex] = giTable
    iIndex += 1
  od
  kOut tabmorph 0, 0, 0, 0, iTables
endin
instr Matrix
  iTables[][] init 1, 1
  iTables[0][0] = giTable
  kOut tabmorph 0, 0, 0, 0, iTables
endin
</CsInstruments>
<CsScore>
i "Count" 0 .015625 0
i "Count" .03125 .015625 1999
i "Matrix" .0625 .015625
e
</CsScore>
</CsoundSynthesizer>
