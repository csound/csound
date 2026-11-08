<CsTest>
description = "copya2ftab rejects multidimensional sources and oversized offsets"
[expect]
exit = "nonzero"
stderr = ["copya2ftab: expected an initialized one-dimensional array", "copya2ftab: offset out of bounds"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
giTable ftgen 0, 0, -4, -2, 0
instr InitShape
  iInput[][] init 1, 1
  copya2ftab iInput, giTable
endin
instr PerfShape
  kInput[][] init 1, 1
  copya2ftab kInput, giTable, 0
endin
instr InitOffset
  iInput[] fillarray 1
  copya2ftab iInput, giTable, 1e30
endin
instr PerfOffset
  kInput[] fillarray 1
  copya2ftab kInput, giTable, 1e30
endin
</CsInstruments>
<CsScore>
i "InitShape" 0 .015625
i "PerfShape" .03125 .015625
i "InitOffset" .0625 .015625
i "PerfOffset" .09375 .015625
e
</CsScore>
</CsoundSynthesizer>
