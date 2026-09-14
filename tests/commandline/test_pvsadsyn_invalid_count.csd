<CsoundSynthesizer>
<CsOptions>
-n -d
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

instr 1
  ain oscili 0.1, 440
  fsig pvsanal ain, 128, 32, 128, 1
  icount = (p4 == -2 ? sqrt:i(-1) : p4)
  aout pvsadsyn fsig, icount, 1
  out aout
endin
</CsInstruments>
<CsScore>
i 1 0 .01 0
i 1 0 .01 -1
i 1 0 .01 66
i 1 0 .01 1e30
i 1 0 .01 -2
</CsScore>
</CsoundSynthesizer>
