<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

giData ftgen 1, 0, -4, -2, 0, 42, 1, 24
instr 1
  kout init 0
  kcount = (p5 == 99 ? sqrt(-1) : p5)
  kindex = (p4 == 99 ? sqrt(-1) : p4)
  tablew kcount, 0, giData
  splitrig 1, kindex, p6, giData, kout
endin
</CsInstruments>
<CsScore>
i 1 0    0.01 -1 1   1
i 1 0.02 0.01  0 0   1
i 1 0.04 0.01  3 1   1
i 1 0.06 0.01  0 2   1
i 1 0.08 0.01  0 5   5
i 1 0.10 0.01  0 0.5 1
i 1 .12 .01 99 1 1
i 1 .14 .01 0 99 1
i 1 .16 .01 1e20 1 1
; A header at the final allocated element has no tick data after it.
i 1 .18 .01 2 1 1
</CsScore>
</CsoundSynthesizer>
