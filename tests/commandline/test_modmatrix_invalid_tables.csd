<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 32
nchnls = 1
0dbfs = 1
instr 1
  iMod ftgen 0, 0, -p6, -2, 0
  iParam ftgen 0, 0, -p7, -2, 0
  iResult ftgen 0, 0, -p8, -2, 0
  iMatrix ftgen 0, 0, -p9, -2, 0
  modmatrix iResult, iMod, iParam, iMatrix, p4, p5, 0
endin
</CsInstruments>
<CsScore>
; Eight init errors: bad dimensions or one undersized table.
i 1 0 .01 0 3 2 3 3 6
i 1 0 .01 2 -1 2 3 3 6
i 1 0 .01 1e30 3 2 3 3 6
i 1 0 .01 2 1e30 2 3 3 6
i 1 0 .01 2 3 1 3 3 6
i 1 0 .01 2 3 2 2 3 6
i 1 0 .01 2 3 2 3 2 6
i 1 0 .01 2 3 2 3 3 5
e
</CsScore>
</CsoundSynthesizer>
