<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

giData ftgen 1, 0, -2, -2, 1, 42
instr 1
  kout init 0
  imax = (p4 == -2 ? sqrt:i(-1) : p4)
  splitrig 1, 0, imax, giData, kout
endin
instr 2
  kfirst init 0
  ksecond init 0
  kthird init 0
  splitrig 1, 0, 1, giData, kfirst, ksecond, kthird
endin
</CsInstruments>
<CsScore>
i 1 0 0.01 0
i 1 0.02 0.01 -1
i 1 0.04 0.01 1e20
i 1 0 .01 -2
i 2 0 .01
</CsScore>
</CsoundSynthesizer>
