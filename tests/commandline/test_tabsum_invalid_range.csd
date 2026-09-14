<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

giTable ftgen 1, 0, -4, -2, 1, 2, 3, 4

instr 1
  kmin = (p4 == 99 ? sqrt(-1) : p4)
  kmax = (p5 == 99 ? sqrt(-1) : p5)
  ksum tabsum 1, kmin, kmax
endin
</CsInstruments>
<CsScore>
i 1 0    0.01 -1 0
i 1 0.02 0.01  0 5
i 1 0.04 0.01  0 1e100
; Values inside the conversion bounds may round outside the table.
i 1 0 .01 -.75 0
i 1 0 .01 0 4.75
i 1 0 .01 99 0
i 1 0 .01 0 99
</CsScore>
</CsoundSynthesizer>
